/* nVidia network driver (based on the Linux driver)
 *
 * Copyright (c) 2010 Zoltan Kovacs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <console.h>
#include <irq.h>
#include <macros.h>
#include <errno.h>
#include <devices.h>
#include <mm/kmalloc.h>
#include <sched/scheduler.h>
#include <network/mii.h>
#include <network/socket.h>
#include <lib/string.h>
#include <lib/random.h>

#include "nvidia.h"
#include "../../bus/pci/pci.h"

#define readl(addr) (*(volatile unsigned int *) (addr))
#define writel(b,addr) (*(volatile unsigned int *) (addr) = (b))
#define wmb() __asm__ __volatile__( "" : : : "memory" )

/*
 * Maximum number of loops until we assume that a bit in the irq mask
 * is stuck. Overridable with module param.
 */
static int max_interrupt_work = 4;

/*
 * Optimization can be either throuput mode or cpu mode
 *
 * Throughput Mode: Every tx and rx packet will generate an interrupt.
 * CPU Mode: Interrupts are controlled by a timer.
 */
enum {
    NV_OPTIMIZATION_MODE_THROUGHPUT,
    NV_OPTIMIZATION_MODE_CPU,
    NV_OPTIMIZATION_MODE_DYNAMIC
};
static int optimization_mode = NV_OPTIMIZATION_MODE_DYNAMIC;

/*
 * Poll interval for timer irq
 *
 * This interval determines how frequent an interrupt is generated.
 * The is value is determined by [(time_in_micro_secs * 100) / (2^10)]
 * Min = 0, and Max = 65535
 */
static int poll_interval = -1;

/*
 * Crossover Detection
 * Realtek 8201 phy + some OEM boards do not work properly.
 */
enum {
    NV_CROSSOVER_DETECTION_DISABLED,
    NV_CROSSOVER_DETECTION_ENABLED
};
static int phy_cross = NV_CROSSOVER_DETECTION_DISABLED;

/*
 * Power down phy when interface is down (persists through reboot;
 * older Linux and other OSes may not power it up again)
 */
static int phy_power_down = 0;

static inline struct fe_priv *get_nvpriv(struct net_device *dev)
{
    return (struct fe_priv* )net_device_get_private( dev );
}

static inline uint8_t* get_hwbase(struct net_device *dev)
{
    return get_nvpriv(dev)->base;
}

static inline void pci_push(uint8_t* base )
{
    /* force out pending posted writes */
    readl(base);
}

static inline uint32_t nv_descr_getlength(struct ring_desc *prd, uint32_t v)
{
    return prd->flaglen & ((v == DESC_VER_1) ? LEN_MASK_V1 : LEN_MASK_V2);
}

static inline uint32_t nv_descr_getlength_ex(struct ring_desc_ex *prd, uint32_t v)
{
    return prd->flaglen & LEN_MASK_V2;
}

static bool nv_optimized(struct fe_priv *np)
{
    if (np->desc_ver == DESC_VER_1 || np->desc_ver == DESC_VER_2)
        return false;
    return true;
}

static int reg_delay(struct net_device *dev, int offset, uint32_t mask, uint32_t target,
                int delay, int delaymax, const char *msg)
{
    uint8_t* base = get_hwbase(dev);

    pci_push(base);
    do {
        udelay(delay);
        delaymax -= delay;
        if (delaymax < 0) {
            if (msg)
                kprintf( INFO, "%s", msg);
            return 1;
        }
    } while ((readl(base + offset) & mask) != target);
    return 0;
}

#define NV_SETUP_RX_RING 0x01
#define NV_SETUP_TX_RING 0x02

static inline uint32_t dma_low(ptr_t addr)
{
    return addr;
}

static inline uint32_t dma_high(ptr_t addr)
{
    return addr>>31>>1; /* 0 if 32bit, shift down by 32 if 64bit */
}

static void setup_hw_rings(struct net_device *dev, int rxtx_flags)
{
    struct fe_priv *np = get_nvpriv(dev);
    uint8_t* base = get_hwbase(dev);

    if (!nv_optimized(np)) {
        if (rxtx_flags & NV_SETUP_RX_RING) {
            writel(dma_low(np->ring_addr), base + NvRegRxRingPhysAddr);
        }
        if (rxtx_flags & NV_SETUP_TX_RING) {
            writel(dma_low(np->ring_addr + np->rx_ring_size*sizeof(struct ring_desc)), base + NvRegTxRingPhysAddr);
        }
    } else {
        if (rxtx_flags & NV_SETUP_RX_RING) {
            writel(dma_low(np->ring_addr), base + NvRegRxRingPhysAddr);
            writel(dma_high(np->ring_addr), base + NvRegRxRingPhysAddrHigh);
        }
        if (rxtx_flags & NV_SETUP_TX_RING) {
            writel(dma_low(np->ring_addr + np->rx_ring_size*sizeof(struct ring_desc_ex)), base + NvRegTxRingPhysAddr);
            writel(dma_high(np->ring_addr + np->rx_ring_size*sizeof(struct ring_desc_ex)), base + NvRegTxRingPhysAddrHigh);
        }
    }
}

static void free_rings(struct net_device *dev)
{
    struct fe_priv *np = get_nvpriv(dev);

    if (!nv_optimized(np)) {
        if (np->rx_ring.orig)
            kfree(np->rx_ring.orig);
    } else {
        if (np->rx_ring.ex)
            kfree(np->rx_ring.ex);
    }

    if (np->rx_skb)
        kfree(np->rx_skb);
    if (np->tx_skb)
        kfree(np->tx_skb);
}

static void nv_txrx_gate(struct net_device *dev, bool gate)
{
    struct fe_priv *np = get_nvpriv(dev);
    uint8_t* base = get_hwbase(dev);
    uint32_t powerstate;

    if (!np->mac_in_use &&
        (np->driver_data & DEV_HAS_POWER_CNTRL)) {
        powerstate = readl(base + NvRegPowerState2);
        if (gate)
            powerstate |= NVREG_POWERSTATE2_GATE_CLOCKS;
        else
            powerstate &= ~NVREG_POWERSTATE2_GATE_CLOCKS;
        writel(powerstate, base + NvRegPowerState2);
    }
}

static void nv_enable_irq(struct net_device *dev)
{
    struct fe_priv *np = get_nvpriv(dev);

    enable_irq(np->dev_irq);
}

static void nv_disable_irq(struct net_device *dev)
{
    struct fe_priv *np = get_nvpriv(dev);

    disable_irq(np->dev_irq);
}

/* In MSIX mode, a write to irqmask behaves as XOR */
static void nv_enable_hw_interrupts(struct net_device *dev, uint32_t mask)
{
    uint8_t* base = get_hwbase(dev);

    writel(mask, base + NvRegIrqMask);
}

static void nv_disable_hw_interrupts(struct net_device *dev, uint32_t mask)
{
    uint8_t* base = get_hwbase(dev);

    writel(0, base + NvRegIrqMask);
}

#define MII_READ    (-1)
/* mii_rw: read/write a register on the PHY.
 *
 * Caller must guarantee serialization
 */
static int mii_rw(struct net_device *dev, int addr, int miireg, int value)
{
    uint8_t* base = get_hwbase(dev);
    uint32_t reg;
    int retval;

    writel(NVREG_MIISTAT_MASK_RW, base + NvRegMIIStatus);

    reg = readl(base + NvRegMIIControl);
    if (reg & NVREG_MIICTL_INUSE) {
        writel(NVREG_MIICTL_INUSE, base + NvRegMIIControl);
        udelay(NV_MIIBUSY_DELAY);
    }

    reg = (addr << NVREG_MIICTL_ADDRSHIFT) | miireg;
    if (value != MII_READ) {
        writel(value, base + NvRegMIIData);
        reg |= NVREG_MIICTL_WRITE;
    }
    writel(reg, base + NvRegMIIControl);

    if (reg_delay(dev, NvRegMIIControl, NVREG_MIICTL_INUSE, 0,
            NV_MIIPHY_DELAY, NV_MIIPHY_DELAYMAX, NULL)) {
        DEBUG_LOG( "nvidia: mii_rw of reg %d at PHY %d timed out.\n",
                miireg, addr);
        retval = -1;
    } else if (value != MII_READ) {
        /* it was a write operation - fewer failures are detectable */
        DEBUG_LOG( "nvidia: mii_rw wrote 0x%x to reg %d at PHY %d\n",
                value, miireg, addr);
        retval = 0;
    } else if (readl(base + NvRegMIIStatus) & NVREG_MIISTAT_ERROR) {
        DEBUG_LOG( "nvidia: mii_rw of reg %d at PHY %d failed.\n",
                miireg, addr);
        retval = -1;
    } else {
        retval = readl(base + NvRegMIIData);
        DEBUG_LOG( "nvidia: mii_rw read from reg %d at PHY %d: 0x%x.\n",
                miireg, addr, retval);
    }

    return retval;
}

static int phy_reset(struct net_device *dev, uint32_t bmcr_setup)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint32_t miicontrol;
    unsigned int tries = 0;

    miicontrol = BMCR_RESET | bmcr_setup;
    if (mii_rw(dev, np->phyaddr, MII_BMCR, miicontrol)) {
        return -1;
    }

    /* wait for 500ms */
    thread_sleep( 500 * 1000 );

    /* must wait till reset is deasserted */
    while (miicontrol & BMCR_RESET) {
        thread_sleep( 10 * 1000 );
        miicontrol = mii_rw(dev, np->phyaddr, MII_BMCR, MII_READ);
        /* FIXME: 100 tries seem excessive */
        if (tries++ > 100)
            return -1;
    }
    return 0;
}

static int phy_init(struct net_device *dev) {
    struct fe_priv *np = get_nvpriv(dev);
    uint8_t* base = get_hwbase(dev);
    uint32_t phyinterface, phy_reserved, mii_status, mii_control, mii_control_1000,reg;

    /* phy errata for E3016 phy */
    if (np->phy_model == PHY_MODEL_MARVELL_E3016) {
        reg = mii_rw(dev, np->phyaddr, MII_NCONFIG, MII_READ);
        reg &= ~PHY_MARVELL_E3016_INITMASK;
        if (mii_rw(dev, np->phyaddr, MII_NCONFIG, reg)) {
            kprintf( INFO, "nvidia: phy write to errata reg failed.\n" );
            return PHY_ERROR;
        }
    }
    if (np->phy_oui == PHY_OUI_REALTEK) {
        if (np->phy_model == PHY_MODEL_REALTEK_8211 &&
            np->phy_rev == PHY_REV_REALTEK_8211B) {
            if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT1)) {
                kprintf( INFO, "nvidia: phy init failed.\n" );
                return PHY_ERROR;
            }
            if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG2, PHY_REALTEK_INIT2)) {
                kprintf( INFO, "nvidia: phy init failed.\n" );
                return PHY_ERROR;
            }
            if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT3)) {
                kprintf( INFO, "nvidia: phy init failed.\n" );
                return PHY_ERROR;
            }
            if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG3, PHY_REALTEK_INIT4)) {
                kprintf( INFO, "nvidia: phy init failed.\n" );
                return PHY_ERROR;
            }
            if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG4, PHY_REALTEK_INIT5)) {
                kprintf( INFO, "nvidia: phy init failed.\n" );
                return PHY_ERROR;
            }
            if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG5, PHY_REALTEK_INIT6)) {
                kprintf( INFO, "nvidia: phy init failed.\n" );
                return PHY_ERROR;
            }
            if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT1)) {
                kprintf( INFO, "nvidia: phy init failed.\n" );
                return PHY_ERROR;
            }
        }
        if (np->phy_model == PHY_MODEL_REALTEK_8211 &&
            np->phy_rev == PHY_REV_REALTEK_8211C) {
            uint32_t powerstate = readl(base + NvRegPowerState2);

            /* need to perform hw phy reset */
            powerstate |= NVREG_POWERSTATE2_PHY_RESET;
            writel(powerstate, base + NvRegPowerState2);
            thread_sleep(25 * 1000);

            powerstate &= ~NVREG_POWERSTATE2_PHY_RESET;
            writel(powerstate, base + NvRegPowerState2);
            thread_sleep(25 * 1000);

            reg = mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG6, MII_READ);
            reg |= PHY_REALTEK_INIT9;
            if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG6, reg)) {
                kprintf( INFO, "nvidia: phy init failed.\n" );
                return PHY_ERROR;
            }
            if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT10)) {
                kprintf( INFO, "nvidia: phy init failed.\n" );
                return PHY_ERROR;
            }
            reg = mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG7, MII_READ);
            if (!(reg & PHY_REALTEK_INIT11)) {
                reg |= PHY_REALTEK_INIT11;
                if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG7, reg)) {
                    kprintf( INFO, "nvidia: phy init failed.\n" );
                    return PHY_ERROR;
                }
            }
            if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT1)) {
                kprintf( INFO, "nvidia: phy init failed.\n" );
                return PHY_ERROR;
            }
        }
        if (np->phy_model == PHY_MODEL_REALTEK_8201) {
            if (np->driver_data & DEV_NEED_PHY_INIT_FIX) {
                phy_reserved = mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG6, MII_READ);
                phy_reserved |= PHY_REALTEK_INIT7;
                if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG6, phy_reserved)) {
                    kprintf( INFO, "nvidia: phy init failed.\n" );
                    return PHY_ERROR;
                }
            }
        }
    }

    /* set advertise register */
    reg = mii_rw(dev, np->phyaddr, MII_ADVERTISE, MII_READ);
    reg |= (ADVERTISE_10HALF|ADVERTISE_10FULL|ADVERTISE_100HALF|ADVERTISE_100FULL|ADVERTISE_PAUSE_ASYM|ADVERTISE_PAUSE_CAP);
    if (mii_rw(dev, np->phyaddr, MII_ADVERTISE, reg)) {
        kprintf( INFO, "nvidia: phy write to advertise failed.\n" );
        return PHY_ERROR;
    }

    /* get phy interface type */
    phyinterface = readl(base + NvRegPhyInterface);

    /* see if gigabit phy */
    mii_status = mii_rw(dev, np->phyaddr, MII_BMSR, MII_READ);
    if (mii_status & PHY_GIGABIT) {
        np->gigabit = PHY_GIGABIT;
        mii_control_1000 = mii_rw(dev, np->phyaddr, MII_CTRL1000, MII_READ);
        mii_control_1000 &= ~ADVERTISE_1000HALF;
        if (phyinterface & PHY_RGMII)
            mii_control_1000 |= ADVERTISE_1000FULL;
        else
            mii_control_1000 &= ~ADVERTISE_1000FULL;

        if (mii_rw(dev, np->phyaddr, MII_CTRL1000, mii_control_1000)) {
            kprintf( INFO, "nvidia: phy init failed.\n" );
            return PHY_ERROR;
        }
    }
    else
        np->gigabit = 0;

    mii_control = mii_rw(dev, np->phyaddr, MII_BMCR, MII_READ);
    mii_control |= BMCR_ANENABLE;

    if (np->phy_oui == PHY_OUI_REALTEK &&
        np->phy_model == PHY_MODEL_REALTEK_8211 &&
        np->phy_rev == PHY_REV_REALTEK_8211C) {
        /* start autoneg since we already performed hw reset above */
        mii_control |= BMCR_ANRESTART;
        if (mii_rw(dev, np->phyaddr, MII_BMCR, mii_control)) {
            kprintf( INFO, "nvidia: phy init failed\n" );
            return PHY_ERROR;
        }
    } else {
        /* reset the phy
         * (certain phys need bmcr to be setup with reset)
         */
        if (phy_reset(dev, mii_control)) {
            kprintf( INFO, "nvidia: phy reset failed\n" );
            return PHY_ERROR;
        }
    }

    /* phy vendor specific configuration */
    if ((np->phy_oui == PHY_OUI_CICADA) && (phyinterface & PHY_RGMII) ) {
        phy_reserved = mii_rw(dev, np->phyaddr, MII_RESV1, MII_READ);
        phy_reserved &= ~(PHY_CICADA_INIT1 | PHY_CICADA_INIT2);
        phy_reserved |= (PHY_CICADA_INIT3 | PHY_CICADA_INIT4);
        if (mii_rw(dev, np->phyaddr, MII_RESV1, phy_reserved)) {
            kprintf( INFO, "nvidia: phy init failed.\n" );
            return PHY_ERROR;
        }
        phy_reserved = mii_rw(dev, np->phyaddr, MII_NCONFIG, MII_READ);
        phy_reserved |= PHY_CICADA_INIT5;
        if (mii_rw(dev, np->phyaddr, MII_NCONFIG, phy_reserved)) {
            kprintf( INFO, "nvidia: phy init failed.\n" );
            return PHY_ERROR;
        }
    }
    if (np->phy_oui == PHY_OUI_CICADA) {
        phy_reserved = mii_rw(dev, np->phyaddr, MII_SREVISION, MII_READ);
        phy_reserved |= PHY_CICADA_INIT6;
        if (mii_rw(dev, np->phyaddr, MII_SREVISION, phy_reserved)) {
            kprintf( INFO, "nvidia: phy init failed.\n" );
            return PHY_ERROR;
        }
    }
    if (np->phy_oui == PHY_OUI_VITESSE) {
        if (mii_rw(dev, np->phyaddr, PHY_VITESSE_INIT_REG1, PHY_VITESSE_INIT1)) {
            kprintf( INFO, "nvidia: phy init failed.\n" );
            return PHY_ERROR;
        }
        if (mii_rw(dev, np->phyaddr, PHY_VITESSE_INIT_REG2, PHY_VITESSE_INIT2)) {
            kprintf( INFO, "nvidia: phy init failed.\n" );
            return PHY_ERROR;
        }
        phy_reserved = mii_rw(dev, np->phyaddr, PHY_VITESSE_INIT_REG4, MII_READ);
        if (mii_rw(dev, np->phyaddr, PHY_VITESSE_INIT_REG4, phy_reserved)) {
            kprintf( INFO, "nvidia: phy init failed.\n" );
            return PHY_ERROR;
        }
        phy_reserved = mii_rw(dev, np->phyaddr, PHY_VITESSE_INIT_REG3, MII_READ);
        phy_reserved &= ~PHY_VITESSE_INIT_MSK1;
        phy_reserved |= PHY_VITESSE_INIT3;
        if (mii_rw(dev, np->phyaddr, PHY_VITESSE_INIT_REG3, phy_reserved)) {
            kprintf( INFO, "nvidia: phy init failed.\n" );
            return PHY_ERROR;
        }
        if (mii_rw(dev, np->phyaddr, PHY_VITESSE_INIT_REG2, PHY_VITESSE_INIT4)) {
            kprintf( INFO, "nvidia: phy init failed.\n" );
            return PHY_ERROR;
        }
        if (mii_rw(dev, np->phyaddr, PHY_VITESSE_INIT_REG2, PHY_VITESSE_INIT5)) {
            kprintf( INFO, "nvidia: phy init failed.\n" );
            return PHY_ERROR;
        }
        phy_reserved = mii_rw(dev, np->phyaddr, PHY_VITESSE_INIT_REG4, MII_READ);
        phy_reserved &= ~PHY_VITESSE_INIT_MSK1;
        phy_reserved |= PHY_VITESSE_INIT3;
        if (mii_rw(dev, np->phyaddr, PHY_VITESSE_INIT_REG4, phy_reserved)) {
            kprintf( INFO, "nvidia: phy init failed.\n" );
            return PHY_ERROR;
        }
        phy_reserved = mii_rw(dev, np->phyaddr, PHY_VITESSE_INIT_REG3, MII_READ);
        if (mii_rw(dev, np->phyaddr, PHY_VITESSE_INIT_REG3, phy_reserved)) {
            kprintf( INFO, "nvidia: phy init failed.\n" );
            return PHY_ERROR;
        }
        if (mii_rw(dev, np->phyaddr, PHY_VITESSE_INIT_REG2, PHY_VITESSE_INIT6)) {
            kprintf( INFO, "nvidia: phy init failed.\n" );
            return PHY_ERROR;
        }
        if (mii_rw(dev, np->phyaddr, PHY_VITESSE_INIT_REG2, PHY_VITESSE_INIT7)) {
            kprintf( INFO, "nvidia: phy init failed.\n" );
            return PHY_ERROR;
        }
        phy_reserved = mii_rw(dev, np->phyaddr, PHY_VITESSE_INIT_REG4, MII_READ);
        if (mii_rw(dev, np->phyaddr, PHY_VITESSE_INIT_REG4, phy_reserved)) {
            kprintf( INFO, "nvidia: phy init failed.\n" );
            return PHY_ERROR;
        }
        phy_reserved = mii_rw(dev, np->phyaddr, PHY_VITESSE_INIT_REG3, MII_READ);
        phy_reserved &= ~PHY_VITESSE_INIT_MSK2;
        phy_reserved |= PHY_VITESSE_INIT8;
        if (mii_rw(dev, np->phyaddr, PHY_VITESSE_INIT_REG3, phy_reserved)) {
            kprintf( INFO, "nvidia: phy init failed.\n" );
            return PHY_ERROR;
        }
        if (mii_rw(dev, np->phyaddr, PHY_VITESSE_INIT_REG2, PHY_VITESSE_INIT9)) {
            kprintf( INFO, "nvidia: phy init failed.\n" );
            return PHY_ERROR;
        }
        if (mii_rw(dev, np->phyaddr, PHY_VITESSE_INIT_REG1, PHY_VITESSE_INIT10)) {
            kprintf( INFO, "nvidia: phy init failed.\n" );
            return PHY_ERROR;
        }
    }
    if (np->phy_oui == PHY_OUI_REALTEK) {
        if (np->phy_model == PHY_MODEL_REALTEK_8211 &&
            np->phy_rev == PHY_REV_REALTEK_8211B) {
            /* reset could have cleared these out, set them back */
            if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT1)) {
                kprintf( INFO, "nvidia: phy init failed.\n" );
                return PHY_ERROR;
            }
            if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG2, PHY_REALTEK_INIT2)) {
                kprintf( INFO, "nvidia: phy init failed.\n" );
                return PHY_ERROR;
            }
            if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT3)) {
                kprintf( INFO, "nvidia: phy init failed.\n" );
                return PHY_ERROR;
            }
            if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG3, PHY_REALTEK_INIT4)) {
                kprintf( INFO, "nvidia: phy init failed.\n" );
                return PHY_ERROR;
            }
            if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG4, PHY_REALTEK_INIT5)) {
                kprintf( INFO, "nvidia: phy init failed.\n" );
                return PHY_ERROR;
            }
            if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG5, PHY_REALTEK_INIT6)) {
                kprintf( INFO, "nvidia: phy init failed.\n" );
                return PHY_ERROR;
            }
            if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT1)) {
                kprintf( INFO, "nvidia: phy init failed.\n" );
                return PHY_ERROR;
            }
        }
        if (np->phy_model == PHY_MODEL_REALTEK_8201) {
            if (np->driver_data & DEV_NEED_PHY_INIT_FIX) {
                phy_reserved = mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG6, MII_READ);
                phy_reserved |= PHY_REALTEK_INIT7;
                if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG6, phy_reserved)) {
                    kprintf( INFO, "nvidia: phy init failed.\n" );
                    return PHY_ERROR;
                }
            }
            if (phy_cross == NV_CROSSOVER_DETECTION_DISABLED) {
                if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT3)) {
                    kprintf( INFO, "nvidia: phy init failed.\n" );
                    return PHY_ERROR;
                }
                phy_reserved = mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG2, MII_READ);
                phy_reserved &= ~PHY_REALTEK_INIT_MSK1;
                phy_reserved |= PHY_REALTEK_INIT3;
                if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG2, phy_reserved)) {
                    kprintf( INFO, "nvidia: phy init failed.\n" );
                    return PHY_ERROR;
                }
                if (mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT1)) {
                    kprintf( INFO, "nvidia: phy init failed.\n" );
                    return PHY_ERROR;
                }
            }
        }
    }

    /* some phys clear out pause advertisment on reset, set it back */
    mii_rw(dev, np->phyaddr, MII_ADVERTISE, reg);

    /* restart auto negotiation, power down phy */
    mii_control = mii_rw(dev, np->phyaddr, MII_BMCR, MII_READ);
    mii_control |= (BMCR_ANRESTART | BMCR_ANENABLE);
    if (phy_power_down) {
        mii_control |= BMCR_PDOWN;
    }
    if (mii_rw(dev, np->phyaddr, MII_BMCR, mii_control)) {
        return PHY_ERROR;
    }

    return 0;
}

static void nv_start_rx(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base = get_hwbase(dev);
    uint32_t rx_ctrl = readl(base + NvRegReceiverControl);

    /* Already running? Stop it. */
    if ((readl(base + NvRegReceiverControl) & NVREG_RCVCTL_START) && !np->mac_in_use) {
        rx_ctrl &= ~NVREG_RCVCTL_START;
        writel(rx_ctrl, base + NvRegReceiverControl);
        pci_push(base);
    }
    writel(np->linkspeed, base + NvRegLinkSpeed);
    pci_push(base);
        rx_ctrl |= NVREG_RCVCTL_START;
        if (np->mac_in_use)
        rx_ctrl &= ~NVREG_RCVCTL_RX_PATH_EN;
    writel(rx_ctrl, base + NvRegReceiverControl);
    pci_push(base);
}

static void nv_stop_rx(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base = get_hwbase(dev);
    uint32_t rx_ctrl = readl(base + NvRegReceiverControl);

    if (!np->mac_in_use)
        rx_ctrl &= ~NVREG_RCVCTL_START;
    else
        rx_ctrl |= NVREG_RCVCTL_RX_PATH_EN;
    writel(rx_ctrl, base + NvRegReceiverControl);
    reg_delay(dev, NvRegReceiverStatus, NVREG_RCVSTAT_BUSY, 0,
            NV_RXSTOP_DELAY1, NV_RXSTOP_DELAY1MAX,
            "nv_stop_rx: ReceiverStatus remained busy");

    udelay(NV_RXSTOP_DELAY2);
    if (!np->mac_in_use)
        writel(0, base + NvRegLinkSpeed);
}

static void nv_start_tx(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base = get_hwbase(dev);
    uint32_t tx_ctrl = readl(base + NvRegTransmitterControl);

    tx_ctrl |= NVREG_XMITCTL_START;
    if (np->mac_in_use)
        tx_ctrl &= ~NVREG_XMITCTL_TX_PATH_EN;
    writel(tx_ctrl, base + NvRegTransmitterControl);
    pci_push(base);
}

static void nv_stop_tx(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base = get_hwbase(dev);
    uint32_t tx_ctrl = readl(base + NvRegTransmitterControl);

    if (!np->mac_in_use)
        tx_ctrl &= ~NVREG_XMITCTL_START;
    else
        tx_ctrl |= NVREG_XMITCTL_TX_PATH_EN;
    writel(tx_ctrl, base + NvRegTransmitterControl);
    reg_delay(dev, NvRegTransmitterStatus, NVREG_XMITSTAT_BUSY, 0,
            NV_TXSTOP_DELAY1, NV_TXSTOP_DELAY1MAX,
            "nv_stop_tx: TransmitterStatus remained busy");

    udelay(NV_TXSTOP_DELAY2);
    if (!np->mac_in_use)
        writel(readl(base + NvRegTransmitPoll) & NVREG_TRANSMITPOLL_MAC_ADDR_REV,
               base + NvRegTransmitPoll);
}

static void nv_start_rxtx(struct net_device *dev)
{
    nv_start_rx(dev);
    nv_start_tx(dev);
}

static void nv_stop_rxtx(struct net_device *dev)
{
    nv_stop_rx(dev);
    nv_stop_tx(dev);
}

static void nv_txrx_reset(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base = get_hwbase(dev);

    writel(NVREG_TXRXCTL_BIT2 | NVREG_TXRXCTL_RESET | np->txrxctl_bits, base + NvRegTxRxControl);
    pci_push(base);
    udelay(NV_TXRX_RESET_DELAY);
    writel(NVREG_TXRXCTL_BIT2 | np->txrxctl_bits, base + NvRegTxRxControl);
    pci_push(base);
}

static void nv_mac_reset(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base = get_hwbase(dev);
    uint32_t temp1, temp2, temp3;

    writel(NVREG_TXRXCTL_BIT2 | NVREG_TXRXCTL_RESET | np->txrxctl_bits, base + NvRegTxRxControl);
    pci_push(base);

    /* save registers since they will be cleared on reset */
    temp1 = readl(base + NvRegMacAddrA);
    temp2 = readl(base + NvRegMacAddrB);
    temp3 = readl(base + NvRegTransmitPoll);

    writel(NVREG_MAC_RESET_ASSERT, base + NvRegMacReset);
    pci_push(base);
    udelay(NV_MAC_RESET_DELAY);
    writel(0, base + NvRegMacReset);
    pci_push(base);
    udelay(NV_MAC_RESET_DELAY);

    /* restore saved registers */
    writel(temp1, base + NvRegMacAddrA);
    writel(temp2, base + NvRegMacAddrB);
    writel(temp3, base + NvRegTransmitPoll);

    writel(NVREG_TXRXCTL_BIT2 | np->txrxctl_bits, base + NvRegTxRxControl);
    pci_push(base);
}

static void nv_get_hw_stats(struct net_device *dev)
{
#if 0
    struct fe_priv *np = netdev_priv(dev);
    u8 __iomem *base = get_hwbase(dev);

    np->estats.tx_bytes += readl(base + NvRegTxCnt);
    np->estats.tx_zero_rexmt += readl(base + NvRegTxZeroReXmt);
    np->estats.tx_one_rexmt += readl(base + NvRegTxOneReXmt);
    np->estats.tx_many_rexmt += readl(base + NvRegTxManyReXmt);
    np->estats.tx_late_collision += readl(base + NvRegTxLateCol);
    np->estats.tx_fifo_errors += readl(base + NvRegTxUnderflow);
    np->estats.tx_carrier_errors += readl(base + NvRegTxLossCarrier);
    np->estats.tx_excess_deferral += readl(base + NvRegTxExcessDef);
    np->estats.tx_retry_error += readl(base + NvRegTxRetryErr);
    np->estats.rx_frame_error += readl(base + NvRegRxFrameErr);
    np->estats.rx_extra_byte += readl(base + NvRegRxExtraByte);
    np->estats.rx_late_collision += readl(base + NvRegRxLateCol);
    np->estats.rx_runt += readl(base + NvRegRxRunt);
    np->estats.rx_frame_too_long += readl(base + NvRegRxFrameTooLong);
    np->estats.rx_over_errors += readl(base + NvRegRxOverflow);
    np->estats.rx_crc_errors += readl(base + NvRegRxFCSErr);
    np->estats.rx_frame_align_error += readl(base + NvRegRxFrameAlignErr);
    np->estats.rx_length_error += readl(base + NvRegRxLenErr);
    np->estats.rx_unicast += readl(base + NvRegRxUnicast);
    np->estats.rx_multicast += readl(base + NvRegRxMulticast);
    np->estats.rx_broadcast += readl(base + NvRegRxBroadcast);
    np->estats.rx_packets =
        np->estats.rx_unicast +
        np->estats.rx_multicast +
        np->estats.rx_broadcast;
    np->estats.rx_errors_total =
        np->estats.rx_crc_errors +
        np->estats.rx_over_errors +
        np->estats.rx_frame_error +
        (np->estats.rx_frame_align_error - np->estats.rx_extra_byte) +
        np->estats.rx_late_collision +
        np->estats.rx_runt +
        np->estats.rx_frame_too_long;
    np->estats.tx_errors_total =
        np->estats.tx_late_collision +
        np->estats.tx_fifo_errors +
        np->estats.tx_carrier_errors +
        np->estats.tx_excess_deferral +
        np->estats.tx_retry_error;

    if (np->driver_data & DEV_HAS_STATISTICS_V2) {
        np->estats.tx_deferral += readl(base + NvRegTxDef);
        np->estats.tx_packets += readl(base + NvRegTxFrame);
        np->estats.rx_bytes += readl(base + NvRegRxCnt);
        np->estats.tx_pause += readl(base + NvRegTxPause);
        np->estats.rx_pause += readl(base + NvRegRxPause);
        np->estats.rx_drop_frame += readl(base + NvRegRxDropFrame);
    }

    if (np->driver_data & DEV_HAS_STATISTICS_V3) {
        np->estats.tx_unicast += readl(base + NvRegTxUnicast);
        np->estats.tx_multicast += readl(base + NvRegTxMulticast);
        np->estats.tx_broadcast += readl(base + NvRegTxBroadcast);
    }
#endif
}

/*
 * nv_get_stats: dev->get_stats function
 * Get latest stats value from the nic.
 * Called with read_lock(&dev_base_lock) held for read -
 * only synchronized against unregister_netdevice.
 */
static struct net_device_stats *nv_get_stats(struct net_device *dev)
{
#if 0
    struct fe_priv *np = net_device_get_private(dev);

    /* If the nic supports hw counters then retrieve latest values */
    if (np->driver_data & (DEV_HAS_STATISTICS_V1|DEV_HAS_STATISTICS_V2|DEV_HAS_STATISTICS_V3)) {
        nv_get_hw_stats(dev);

        /* copy to net_device stats */
        dev->stats.tx_bytes = np->estats.tx_bytes;
        dev->stats.tx_fifo_errors = np->estats.tx_fifo_errors;
        dev->stats.tx_carrier_errors = np->estats.tx_carrier_errors;
        dev->stats.rx_crc_errors = np->estats.rx_crc_errors;
        dev->stats.rx_over_errors = np->estats.rx_over_errors;
        dev->stats.rx_errors = np->estats.rx_errors_total;
        dev->stats.tx_errors = np->estats.tx_errors_total;
    }
#endif

    return &dev->stats;
}

/*
 * nv_alloc_rx: fill rx ring entries.
 * Return 1 if the allocations for the skbs failed and the
 * rx engine is without Available descriptors
 */
static int nv_alloc_rx(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    struct ring_desc* less_rx;

    less_rx = np->get_rx.orig;
    if (less_rx-- == np->first_rx.orig)
        less_rx = np->last_rx.orig;

    while (np->put_rx.orig != less_rx) {
        packet_t* skb = create_packet( np->rx_buf_sz + NV_RX_ALLOC_PAD );
        if (skb) {
            np->put_rx_ctx->skb = skb;
            np->put_rx_ctx->dma = ( ptr_t )skb->data;
            np->put_rx_ctx->dma_len = skb->size;
            np->put_rx.orig->buf = np->put_rx_ctx->dma;
            wmb();
            np->put_rx.orig->flaglen = np->rx_buf_sz | NV_RX_AVAIL;
            if (__unlikely(np->put_rx.orig++ == np->last_rx.orig))
                np->put_rx.orig = np->first_rx.orig;
            if (__unlikely(np->put_rx_ctx++ == np->last_rx_ctx))
                np->put_rx_ctx = np->first_rx_ctx;
        } else {
            return 1;
        }
    }
    return 0;
}

static int nv_alloc_rx_optimized(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    struct ring_desc_ex* less_rx;

    less_rx = np->get_rx.ex;
    if (less_rx-- == np->first_rx.ex)
        less_rx = np->last_rx.ex;

    while (np->put_rx.ex != less_rx) {
        packet_t* skb = create_packet( np->rx_buf_sz + NV_RX_ALLOC_PAD );
        if (skb) {
            np->put_rx_ctx->skb = skb;
            np->put_rx_ctx->dma = ( ptr_t )skb->data;
            np->put_rx_ctx->dma_len = skb->size;
            np->put_rx.ex->bufhigh = dma_high(np->put_rx_ctx->dma);
            np->put_rx.ex->buflow = dma_low(np->put_rx_ctx->dma);
            wmb();
            np->put_rx.ex->flaglen = np->rx_buf_sz | NV_RX2_AVAIL;
            if (__unlikely(np->put_rx.ex++ == np->last_rx.ex))
                np->put_rx.ex = np->first_rx.ex;
            if (__unlikely(np->put_rx_ctx++ == np->last_rx_ctx))
                np->put_rx_ctx = np->first_rx_ctx;
        } else {
            return 1;
        }
    }
    return 0;
}

/* If rx bufs are exhausted called after 50ms to attempt to refresh */

static int nv_do_rx_refill(void* data)
{
    struct net_device *dev = (struct net_device *) data;
    struct fe_priv *np = net_device_get_private(dev);
    int retcode;

    disable_irq(np->dev_irq);

    if (!nv_optimized(np))
        retcode = nv_alloc_rx(dev);
    else
        retcode = nv_alloc_rx_optimized(dev);

    if (retcode) {
        spinlock_disable(&np->lock);
        if (!np->in_shutdown)
            timer_setup(&np->oom_kick, get_system_time() + OOM_REFILL);
        spinunlock_enable(&np->lock);
    }

    enable_irq(np->dev_irq);

    return 0;
}

static void nv_init_rx(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    int i;

    np->get_rx = np->put_rx = np->first_rx = np->rx_ring;

    if (!nv_optimized(np))
        np->last_rx.orig = &np->rx_ring.orig[np->rx_ring_size-1];
    else
        np->last_rx.ex = &np->rx_ring.ex[np->rx_ring_size-1];
    np->get_rx_ctx = np->put_rx_ctx = np->first_rx_ctx = np->rx_skb;
    np->last_rx_ctx = &np->rx_skb[np->rx_ring_size-1];

    for (i = 0; i < np->rx_ring_size; i++) {
        if (!nv_optimized(np)) {
            np->rx_ring.orig[i].flaglen = 0;
            np->rx_ring.orig[i].buf = 0;
        } else {
            np->rx_ring.ex[i].flaglen = 0;
            np->rx_ring.ex[i].txvlan = 0;
            np->rx_ring.ex[i].bufhigh = 0;
            np->rx_ring.ex[i].buflow = 0;
        }
        np->rx_skb[i].skb = NULL;
        np->rx_skb[i].dma = 0;
    }
}

static void nv_init_tx(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    int i;

    np->get_tx = np->put_tx = np->first_tx = np->tx_ring;

    if (!nv_optimized(np))
        np->last_tx.orig = &np->tx_ring.orig[np->tx_ring_size-1];
    else
        np->last_tx.ex = &np->tx_ring.ex[np->tx_ring_size-1];
    np->get_tx_ctx = np->put_tx_ctx = np->first_tx_ctx = np->tx_skb;
    np->last_tx_ctx = &np->tx_skb[np->tx_ring_size-1];
    np->tx_pkts_in_progress = 0;
    np->tx_change_owner = NULL;
    np->tx_end_flip = NULL;
    np->tx_stop = 0;

    for (i = 0; i < np->tx_ring_size; i++) {
        if (!nv_optimized(np)) {
            np->tx_ring.orig[i].flaglen = 0;
            np->tx_ring.orig[i].buf = 0;
        } else {
            np->tx_ring.ex[i].flaglen = 0;
            np->tx_ring.ex[i].txvlan = 0;
            np->tx_ring.ex[i].bufhigh = 0;
            np->tx_ring.ex[i].buflow = 0;
        }
        np->tx_skb[i].skb = NULL;
        np->tx_skb[i].dma = 0;
        np->tx_skb[i].dma_len = 0;
        np->tx_skb[i].dma_single = 0;
        np->tx_skb[i].first_tx_desc = NULL;
        np->tx_skb[i].next_tx_ctx = NULL;
    }
}

static int nv_init_ring(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);

    nv_init_tx(dev);
    nv_init_rx(dev);

    if (!nv_optimized(np))
        return nv_alloc_rx(dev);
    else
        return nv_alloc_rx_optimized(dev);
}

static void nv_unmap_txskb(struct fe_priv *np, struct nv_skb_map *tx_skb)
{
    if (tx_skb->dma) {
        tx_skb->dma = 0;
    }
}

static int nv_release_txskb(struct fe_priv *np, struct nv_skb_map *tx_skb)
{
    nv_unmap_txskb(np, tx_skb);
    if (tx_skb->skb) {
        delete_packet(tx_skb->skb);
        tx_skb->skb = NULL;

        return 1;
    }
    return 0;
}

static void nv_drain_tx(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    unsigned int i;

    for (i = 0; i < np->tx_ring_size; i++) {
        if (!nv_optimized(np)) {
            np->tx_ring.orig[i].flaglen = 0;
            np->tx_ring.orig[i].buf = 0;
        } else {
            np->tx_ring.ex[i].flaglen = 0;
            np->tx_ring.ex[i].txvlan = 0;
            np->tx_ring.ex[i].bufhigh = 0;
            np->tx_ring.ex[i].buflow = 0;
        }

        if (nv_release_txskb(np, &np->tx_skb[i])) {
            // todo giszo dev->stats.tx_dropped++;
        }

        np->tx_skb[i].dma = 0;
        np->tx_skb[i].dma_len = 0;
        np->tx_skb[i].dma_single = 0;
        np->tx_skb[i].first_tx_desc = NULL;
        np->tx_skb[i].next_tx_ctx = NULL;
    }
    np->tx_pkts_in_progress = 0;
    np->tx_change_owner = NULL;
    np->tx_end_flip = NULL;
}

static void nv_drain_rx(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    int i;

    for (i = 0; i < np->rx_ring_size; i++) {
        if (!nv_optimized(np)) {
            np->rx_ring.orig[i].flaglen = 0;
            np->rx_ring.orig[i].buf = 0;
        } else {
            np->rx_ring.ex[i].flaglen = 0;
            np->rx_ring.ex[i].txvlan = 0;
            np->rx_ring.ex[i].bufhigh = 0;
            np->rx_ring.ex[i].buflow = 0;
        }
        wmb();
        if (np->rx_skb[i].skb) {
            delete_packet(np->rx_skb[i].skb);
            np->rx_skb[i].skb = NULL;
        }
    }
}

static void nv_drain_rxtx(struct net_device *dev)
{
    nv_drain_tx(dev);
    nv_drain_rx(dev);
}

static inline uint32_t nv_get_empty_tx_slots(struct fe_priv *np)
{
    return (uint32_t)(np->tx_ring_size - ((np->tx_ring_size + (np->put_tx_ctx - np->get_tx_ctx)) % np->tx_ring_size));
}

static void nv_legacybackoff_reseed(struct net_device *dev)
{
    uint8_t* base = get_hwbase(dev);
    uint32_t reg;
    uint32_t low;
    int tx_status = 0;

    reg = readl(base + NvRegSlotTime) & ~NVREG_SLOTTIME_MASK;
    random_get_bytes((uint8_t*)&low, sizeof(low));
    reg |= low & NVREG_SLOTTIME_MASK;

    /* Need to stop tx before change takes effect.
     * Caller has already gained np->lock.
     */
    tx_status = readl(base + NvRegTransmitterControl) & NVREG_XMITCTL_START;
    if (tx_status)
        nv_stop_tx(dev);
    nv_stop_rx(dev);
    writel(reg, base + NvRegSlotTime);
    if (tx_status)
        nv_start_tx(dev);
    nv_start_rx(dev);
}

/* Gear Backoff Seeds */
#define BACKOFF_SEEDSET_ROWS    8
#define BACKOFF_SEEDSET_LFSRS   15

/* Known Good seed sets */
static const uint32_t main_seedset[BACKOFF_SEEDSET_ROWS][BACKOFF_SEEDSET_LFSRS] = {
    {145, 155, 165, 175, 185, 196, 235, 245, 255, 265, 275, 285, 660, 690, 874},
    {245, 255, 265, 575, 385, 298, 335, 345, 355, 366, 375, 385, 761, 790, 974},
    {145, 155, 165, 175, 185, 196, 235, 245, 255, 265, 275, 285, 660, 690, 874},
    {245, 255, 265, 575, 385, 298, 335, 345, 355, 366, 375, 386, 761, 790, 974},
    {266, 265, 276, 585, 397, 208, 345, 355, 365, 376, 385, 396, 771, 700, 984},
    {266, 265, 276, 586, 397, 208, 346, 355, 365, 376, 285, 396, 771, 700, 984},
    {366, 365, 376, 686, 497, 308, 447, 455, 466, 476, 485, 496, 871, 800,  84},
    {466, 465, 476, 786, 597, 408, 547, 555, 566, 576, 585, 597, 971, 900, 184}};

static const uint32_t gear_seedset[BACKOFF_SEEDSET_ROWS][BACKOFF_SEEDSET_LFSRS] = {
    {251, 262, 273, 324, 319, 508, 375, 364, 341, 371, 398, 193, 375,  30, 295},
    {351, 375, 373, 469, 551, 639, 477, 464, 441, 472, 498, 293, 476, 130, 395},
    {351, 375, 373, 469, 551, 639, 477, 464, 441, 472, 498, 293, 476, 130, 397},
    {251, 262, 273, 324, 319, 508, 375, 364, 341, 371, 398, 193, 375,  30, 295},
    {251, 262, 273, 324, 319, 508, 375, 364, 341, 371, 398, 193, 375,  30, 295},
    {351, 375, 373, 469, 551, 639, 477, 464, 441, 472, 498, 293, 476, 130, 395},
    {351, 375, 373, 469, 551, 639, 477, 464, 441, 472, 498, 293, 476, 130, 395},
    {351, 375, 373, 469, 551, 639, 477, 464, 441, 472, 498, 293, 476, 130, 395}};

static void nv_gear_backoff_reseed(struct net_device *dev)
{
    uint8_t* base = get_hwbase(dev);
    uint32_t miniseed1, miniseed2, miniseed2_reversed, miniseed3, miniseed3_reversed;
    uint32_t temp, seedset, combinedSeed;
    int i;

    /* Setup seed for free running LFSR */
    /* We are going to read the time stamp counter 3 times
       and swizzle bits around to increase randomness */
    random_get_bytes((uint8_t*)&miniseed1, sizeof(miniseed1));
    miniseed1 &= 0x0fff;
    if (miniseed1 == 0)
        miniseed1 = 0xabc;

    random_get_bytes((uint8_t*)&miniseed2, sizeof(miniseed2));
    miniseed2 &= 0x0fff;
    if (miniseed2 == 0)
        miniseed2 = 0xabc;
    miniseed2_reversed =
        ((miniseed2 & 0xF00) >> 8) |
         (miniseed2 & 0x0F0) |
         ((miniseed2 & 0x00F) << 8);

    random_get_bytes((uint8_t*)&miniseed3, sizeof(miniseed3));
    miniseed3 &= 0x0fff;
    if (miniseed3 == 0)
        miniseed3 = 0xabc;
    miniseed3_reversed =
        ((miniseed3 & 0xF00) >> 8) |
         (miniseed3 & 0x0F0) |
         ((miniseed3 & 0x00F) << 8);

    combinedSeed = ((miniseed1 ^ miniseed2_reversed) << 12) |
               (miniseed2 ^ miniseed3_reversed);

    /* Seeds can not be zero */
    if ((combinedSeed & NVREG_BKOFFCTRL_SEED_MASK) == 0)
        combinedSeed |= 0x08;
    if ((combinedSeed & (NVREG_BKOFFCTRL_SEED_MASK << NVREG_BKOFFCTRL_GEAR)) == 0)
        combinedSeed |= 0x8000;

    /* No need to disable tx here */
    temp = NVREG_BKOFFCTRL_DEFAULT | (0 << NVREG_BKOFFCTRL_SELECT);
    temp |= combinedSeed & NVREG_BKOFFCTRL_SEED_MASK;
    temp |= combinedSeed >> NVREG_BKOFFCTRL_GEAR;
    writel(temp,base + NvRegBackOffControl);

        /* Setup seeds for all gear LFSRs. */
    random_get_bytes((uint8_t*)&seedset, sizeof(seedset));
    seedset = seedset % BACKOFF_SEEDSET_ROWS;
    for (i = 1; i <= BACKOFF_SEEDSET_LFSRS; i++)
    {
        temp = NVREG_BKOFFCTRL_DEFAULT | (i << NVREG_BKOFFCTRL_SELECT);
        temp |= main_seedset[seedset][i-1] & 0x3ff;
        temp |= ((gear_seedset[seedset][i-1] & 0x3ff) << NVREG_BKOFFCTRL_GEAR);
        writel(temp, base + NvRegBackOffControl);
    }
}

/*
 * nv_start_xmit: dev->hard_start_xmit function
 * Called with netif_tx_lock held.
 */
static netdev_tx_t nv_start_xmit(struct net_device *dev, packet_t *skb)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint32_t tx_flags = 0;
    uint32_t tx_flags_extra = (np->desc_ver == DESC_VER_1 ? NV_TX_LASTPACKET : NV_TX2_LASTPACKET);
    uint32_t offset = 0;
    uint32_t bcnt;
    uint32_t size = skb->size;
    uint32_t entries = (size >> NV_TX2_TSO_MAX_SHIFT) + ((size & (NV_TX2_TSO_MAX_SIZE-1)) ? 1 : 0);
    uint32_t empty_slots;
    struct ring_desc* put_tx;
    struct ring_desc* start_tx;
    struct ring_desc* prev_tx;
    struct nv_skb_map* prev_tx_ctx;

    spinlock_disable(&np->lock);
    empty_slots = nv_get_empty_tx_slots(np);
    if (__unlikely(empty_slots <= entries)) {
        // todo giszo netif_stop_queue(dev);
        np->tx_stop = 1;

        spinunlock_enable(&np->lock);

        return NETDEV_TX_BUSY;
    }
    spinunlock_enable(&np->lock);

    start_tx = put_tx = np->put_tx.orig;

    /* setup the header buffer */
    do {
        prev_tx = put_tx;
        prev_tx_ctx = np->put_tx_ctx;
        bcnt = (size > NV_TX2_TSO_MAX_SIZE) ? NV_TX2_TSO_MAX_SIZE : size;
        np->put_tx_ctx->dma = ( ptr_t )( skb->data + offset );
        np->put_tx_ctx->dma_len = bcnt;
        np->put_tx_ctx->dma_single = 1;
        put_tx->buf = np->put_tx_ctx->dma;
        put_tx->flaglen = (bcnt-1) | tx_flags;

        tx_flags = np->tx_flags;
        offset += bcnt;
        size -= bcnt;
        if (__unlikely(put_tx++ == np->last_tx.orig))
            put_tx = np->first_tx.orig;
        if (__unlikely(np->put_tx_ctx++ == np->last_tx_ctx))
            np->put_tx_ctx = np->first_tx_ctx;
    } while (size);

    /* set last fragment flag  */
    prev_tx->flaglen |= tx_flags_extra;

    /* save skb in this slot's context area */
    prev_tx_ctx->skb = skb;

    tx_flags_extra = /* todo giszo skb->ip_summed == CHECKSUM_PARTIAL */ 0 ? NV_TX2_CHECKSUM_L3 | NV_TX2_CHECKSUM_L4 : 0;

    spinlock_disable(&np->lock);

    /* set tx flags */
    start_tx->flaglen |= tx_flags | tx_flags_extra;
    np->put_tx.orig = put_tx;

    spinunlock_enable(&np->lock);

    // todo giszo dev->trans_start = jiffies;
    writel(NVREG_TXRXCTL_KICK|np->txrxctl_bits, get_hwbase(dev) + NvRegTxRxControl);

    return NETDEV_TX_OK;
}

static netdev_tx_t nv_start_xmit_optimized(struct net_device* dev, packet_t* skb)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint32_t tx_flags = 0;
    uint32_t tx_flags_extra;
    uint32_t offset = 0;
    uint32_t bcnt;
    uint32_t size = skb->size;
    uint32_t entries = (size >> NV_TX2_TSO_MAX_SHIFT) + ((size & (NV_TX2_TSO_MAX_SIZE-1)) ? 1 : 0);
    uint32_t empty_slots;
    struct ring_desc_ex* put_tx;
    struct ring_desc_ex* start_tx;
    struct ring_desc_ex* prev_tx;
    struct nv_skb_map* prev_tx_ctx;
    struct nv_skb_map* start_tx_ctx;

    spinlock_disable(&np->lock);
    empty_slots = nv_get_empty_tx_slots(np);
    if (__unlikely(empty_slots <= entries)) {
        // todo giszo netif_stop_queue(dev);
        np->tx_stop = 1;

        spinunlock_enable(&np->lock );

        return NETDEV_TX_BUSY;
    }

    spinunlock_enable(&np->lock);

    start_tx = put_tx = np->put_tx.ex;
    start_tx_ctx = np->put_tx_ctx;

    /* setup the header buffer */
    do {
        prev_tx = put_tx;
        prev_tx_ctx = np->put_tx_ctx;
        bcnt = (size > NV_TX2_TSO_MAX_SIZE) ? NV_TX2_TSO_MAX_SIZE : size;
        np->put_tx_ctx->dma = ( ptr_t )( skb->data + offset );
        np->put_tx_ctx->dma_len = bcnt;
        np->put_tx_ctx->dma_single = 1;
        put_tx->bufhigh = dma_high(np->put_tx_ctx->dma);
        put_tx->buflow = dma_low(np->put_tx_ctx->dma);
        put_tx->flaglen = (bcnt-1) | tx_flags;

        tx_flags = NV_TX2_VALID;
        offset += bcnt;
        size -= bcnt;
        if (__unlikely(put_tx++ == np->last_tx.ex))
            put_tx = np->first_tx.ex;
        if (__unlikely(np->put_tx_ctx++ == np->last_tx_ctx))
            np->put_tx_ctx = np->first_tx_ctx;
    } while (size);

    /* set last fragment flag  */
    prev_tx->flaglen |= NV_TX2_LASTPACKET;

    /* save skb in this slot's context area */
    prev_tx_ctx->skb = skb;

    tx_flags_extra = /* skb->ip_summed == CHECKSUM_PARTIAL */ 0 ? NV_TX2_CHECKSUM_L3 | NV_TX2_CHECKSUM_L4 : 0;

    start_tx->txvlan = 0;

    spinlock_disable(&np->lock );

    if (np->tx_limit) {
        /* Limit the number of outstanding tx. Setup all fragments, but
         * do not set the VALID bit on the first descriptor. Save a pointer
         * to that descriptor and also for next skb_map element.
         */

        if (np->tx_pkts_in_progress == NV_TX_LIMIT_COUNT) {
            if (!np->tx_change_owner)
                np->tx_change_owner = start_tx_ctx;

            /* remove VALID bit */
            tx_flags &= ~NV_TX2_VALID;
            start_tx_ctx->first_tx_desc = start_tx;
            start_tx_ctx->next_tx_ctx = np->put_tx_ctx;
            np->tx_end_flip = np->put_tx_ctx;
        } else {
            np->tx_pkts_in_progress++;
        }
    }

    /* set tx flags */
    start_tx->flaglen |= tx_flags | tx_flags_extra;
    np->put_tx.ex = put_tx;

    spinunlock_enable(&np->lock);

    //dev->trans_start = jiffies;
    writel(NVREG_TXRXCTL_KICK|np->txrxctl_bits, get_hwbase(dev) + NvRegTxRxControl);

    return NETDEV_TX_OK;
}

static inline void nv_tx_flip_ownership(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);

    np->tx_pkts_in_progress--;
    if (np->tx_change_owner) {
        np->tx_change_owner->first_tx_desc->flaglen |= NV_TX2_VALID;
        np->tx_pkts_in_progress++;

        np->tx_change_owner = np->tx_change_owner->next_tx_ctx;
        if (np->tx_change_owner == np->tx_end_flip)
            np->tx_change_owner = NULL;

        writel(NVREG_TXRXCTL_KICK|np->txrxctl_bits, get_hwbase(dev) + NvRegTxRxControl);
    }
}

/*
 * nv_tx_done: check for completed packets, release the skbs.
 *
 * Caller must own np->lock.
 */
static int nv_tx_done(struct net_device *dev, int limit)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint32_t flags;
    int tx_work = 0;
    struct ring_desc* orig_get_tx = np->get_tx.orig;

    while ((np->get_tx.orig != np->put_tx.orig) &&
           !((flags = np->get_tx.orig->flaglen) & NV_TX_VALID) &&
           (tx_work < limit)) {

        DEBUG_LOG( "nvidia: nv_tx_done: flags 0x%x.\n", flags );

        nv_unmap_txskb(np, np->get_tx_ctx);

        if (np->desc_ver == DESC_VER_1) {
            if (flags & NV_TX_LASTPACKET) {
                if (flags & NV_TX_ERROR) {
                    if (flags & NV_TX_UNDERFLOW){
                        //dev->stats.tx_fifo_errors++;
                    }
                    if (flags & NV_TX_CARRIERLOST){
                        //dev->stats.tx_carrier_errors++;
                    }
                    if ((flags & NV_TX_RETRYERROR) && !(flags & NV_TX_RETRYCOUNT_MASK))
                        nv_legacybackoff_reseed(dev);
                    //dev->stats.tx_errors++;
                } else {
                    //dev->stats.tx_packets++;
                    //dev->stats.tx_bytes += np->get_tx_ctx->skb->len;
                }
                delete_packet(np->get_tx_ctx->skb);
                np->get_tx_ctx->skb = NULL;
                tx_work++;
            }
        } else {
            if (flags & NV_TX2_LASTPACKET) {
                if (flags & NV_TX2_ERROR) {
                    if (flags & NV_TX2_UNDERFLOW) {
                        //dev->stats.tx_fifo_errors++;
                    }
                    if (flags & NV_TX2_CARRIERLOST) {
                        //dev->stats.tx_carrier_errors++;
                    }
                    if ((flags & NV_TX2_RETRYERROR) && !(flags & NV_TX2_RETRYCOUNT_MASK))
                        nv_legacybackoff_reseed(dev);
                    //dev->stats.tx_errors++;
                } else {
                    //dev->stats.tx_packets++;
                    //dev->stats.tx_bytes += np->get_tx_ctx->skb->len;
                }
                delete_packet(np->get_tx_ctx->skb);
                np->get_tx_ctx->skb = NULL;
                tx_work++;
            }
        }
        if (__unlikely(np->get_tx.orig++ == np->last_tx.orig))
            np->get_tx.orig = np->first_tx.orig;
        if (__unlikely(np->get_tx_ctx++ == np->last_tx_ctx))
            np->get_tx_ctx = np->first_tx_ctx;
    }
    if (__unlikely((np->tx_stop == 1) && (np->get_tx.orig != orig_get_tx))) {
        np->tx_stop = 0;
        // todo giszo netif_wake_queue(dev);
    }
    return tx_work;
}

static int nv_tx_done_optimized(struct net_device *dev, int limit)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint32_t flags;
    int tx_work = 0;
    struct ring_desc_ex* orig_get_tx = np->get_tx.ex;

    while ((np->get_tx.ex != np->put_tx.ex) &&
           !((flags = np->get_tx.ex->flaglen) & NV_TX_VALID) &&
           (tx_work < limit)) {

        DEBUG_LOG( "nvidia: nv_tx_done_optimized: flags 0x%x.\n", flags );

        nv_unmap_txskb(np, np->get_tx_ctx);

        if (flags & NV_TX2_LASTPACKET) {
            if (!(flags & NV_TX2_ERROR)) {
                //dev->stats.tx_packets++;
            } else {
                if ((flags & NV_TX2_RETRYERROR) && !(flags & NV_TX2_RETRYCOUNT_MASK)) {
                    if (np->driver_data & DEV_HAS_GEAR_MODE)
                        nv_gear_backoff_reseed(dev);
                    else
                        nv_legacybackoff_reseed(dev);
                }
            }

            delete_packet(np->get_tx_ctx->skb);
            np->get_tx_ctx->skb = NULL;
            tx_work++;

            if (np->tx_limit) {
                nv_tx_flip_ownership(dev);
            }
        }
        if (__unlikely(np->get_tx.ex++ == np->last_tx.ex))
            np->get_tx.ex = np->first_tx.ex;
        if (__unlikely(np->get_tx_ctx++ == np->last_tx_ctx))
            np->get_tx_ctx = np->first_tx_ctx;
    }
    if (__unlikely((np->tx_stop == 1) && (np->get_tx.ex != orig_get_tx))) {
        np->tx_stop = 0;
        // todo giszo netif_wake_queue(dev);
    }
    return tx_work;
}

/*
 * nv_tx_timeout: dev->tx_timeout function
 * Called with netif_tx_lock held.
 */
static void nv_tx_timeout(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base = get_hwbase(dev);
    uint32_t status;
    union ring_type put_tx;
    int saved_tx_limit;

    status = readl(base + NvRegIrqStatus) & NVREG_IRQSTAT_MASK;

    kprintf( INFO, "nvidia: Got tx_timeout. irq: %08x\n", status);

    {
        int i;

        kprintf( INFO, "nvidia: Ring at %lx\n", (unsigned long)np->ring_addr);
        kprintf( INFO, "nvidia: Dumping tx registers\n" );
        for (i=0;i<=np->register_size;i+= 32) {
            kprintf( INFO, "%3x: %08x %08x %08x %08x %08x %08x %08x %08x\n",
                    i,
                    readl(base + i + 0), readl(base + i + 4),
                    readl(base + i + 8), readl(base + i + 12),
                    readl(base + i + 16), readl(base + i + 20),
                    readl(base + i + 24), readl(base + i + 28));
        }
        kprintf( INFO, "nvidia: Dumping tx ring\n" );
        for (i=0;i<np->tx_ring_size;i+= 4) {
            if (!nv_optimized(np)) {
                kprintf( INFO, "%03x: %08x %08x // %08x %08x // %08x %08x // %08x %08x\n",
                       i,
                       np->tx_ring.orig[i].buf,
                       np->tx_ring.orig[i].flaglen,
                       np->tx_ring.orig[i+1].buf,
                       np->tx_ring.orig[i+1].flaglen,
                       np->tx_ring.orig[i+2].buf,
                       np->tx_ring.orig[i+2].flaglen,
                       np->tx_ring.orig[i+3].buf,
                       np->tx_ring.orig[i+3].flaglen);
            } else {
                kprintf( INFO, "%03x: %08x %08x %08x // %08x %08x %08x // %08x %08x %08x // %08x %08x %08x\n",
                       i,
                       np->tx_ring.ex[i].bufhigh,
                       np->tx_ring.ex[i].buflow,
                       np->tx_ring.ex[i].flaglen,
                       np->tx_ring.ex[i+1].bufhigh,
                       np->tx_ring.ex[i+1].buflow,
                       np->tx_ring.ex[i+1].flaglen,
                       np->tx_ring.ex[i+2].bufhigh,
                       np->tx_ring.ex[i+2].buflow,
                       np->tx_ring.ex[i+2].flaglen,
                       np->tx_ring.ex[i+3].bufhigh,
                       np->tx_ring.ex[i+3].buflow,
                       np->tx_ring.ex[i+3].flaglen);
            }
        }
    }

    spinlock_disable(&np->lock);

    /* 1) stop tx engine */
    nv_stop_tx(dev);

    /* 2) complete any outstanding tx and do not give HW any limited tx pkts */
    saved_tx_limit = np->tx_limit;
    np->tx_limit = 0; /* prevent giving HW any limited pkts */
    np->tx_stop = 0;  /* prevent waking tx queue */
    if (!nv_optimized(np))
        nv_tx_done(dev, np->tx_ring_size);
    else
        nv_tx_done_optimized(dev, np->tx_ring_size);

    /* save current HW postion */
    if (np->tx_change_owner)
        put_tx.ex = np->tx_change_owner->first_tx_desc;
    else
        put_tx = np->put_tx;

    /* 3) clear all tx state */
    nv_drain_tx(dev);
    nv_init_tx(dev);

    /* 4) restore state to current HW position */
    np->get_tx = np->put_tx = put_tx;
    np->tx_limit = saved_tx_limit;

    /* 5) restart tx engine */
    nv_start_tx(dev);
    // todo giszo netif_wake_queue(dev);

    spinunlock_enable(&np->lock);
}

/*
 * Called when the nic notices a mismatch between the actual data len on the
 * wire and the len indicated in the 802 header
 */
static int nv_getlen(struct net_device *dev, void *packet, int datalen)
{
    int hdrlen; /* length of the 802 header */
    int protolen;   /* length as stored in the proto field */

    /* 1) calculate len according to header */
    protolen = ntohw( ( ( ethernet_header_t* )packet )->proto );
    hdrlen = ETH_HEADER_LEN;

    DEBUG_LOG( "nvidia: nv_getlen: datalen %d, protolen %d, hdrlen %d\n",
                datalen, protolen, hdrlen );

    if (protolen > ETH_DATA_LEN)
        return datalen; /* Value in proto field not a len, no checks possible */

    protolen += hdrlen;
    /* consistency checks: */
    if (datalen > ETH_ZLEN) {
        if (datalen >= protolen) {
            /* more data on wire than in 802 header, trim of
             * additional data.
             */
            DEBUG_LOG( "nvidia: nv_getlen: accepting %d bytes.\n", protolen );
            return protolen;
        } else {
            /* less data on wire than mentioned in header.
             * Discard the packet.
             */
            DEBUG_LOG( "nvidia: nv_getlen: discarding long packet.\n" );
            return -1;
        }
    } else {
        /* short packet. Accept only if 802 values are also short */
        if (protolen > ETH_ZLEN) {
            DEBUG_LOG( "nvidia: nv_getlen: discarding short packet.\n" );
            return -1;
        }

        DEBUG_LOG( "nvidia: nv_getlen: accepting %d bytes.\n", datalen);

        return datalen;
    }
}

static int nv_rx_process(struct net_device *dev, int limit)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint32_t flags;
    int rx_work = 0;
    packet_t* skb;
    int len;

    while((np->get_rx.orig != np->put_rx.orig) &&
          !((flags = np->get_rx.orig->flaglen) & NV_RX_AVAIL) &&
        (rx_work < limit)) {

        DEBUG_LOG( "nvidia: nv_rx_process: flags 0x%x.\n",flags);

        /*
         * the packet is for us.
         * TODO: check if a prefetch of the first cacheline improves
         * the performance.
         */
        skb = np->get_rx_ctx->skb;
        np->get_rx_ctx->skb = NULL;

        /* look at what we actually got: */
        if (np->desc_ver == DESC_VER_1) {
            if (__likely(flags & NV_RX_DESCRIPTORVALID)) {
                len = flags & LEN_MASK_V1;
                if (__unlikely(flags & NV_RX_ERROR)) {
                    if ((flags & NV_RX_ERROR_MASK) == NV_RX_ERROR4) {
                        len = nv_getlen(dev, skb->data, len);
                        if (len < 0) {
                            //dev->stats.rx_errors++;
                            delete_packet(skb);
                            goto next_pkt;
                        }
                    }
                    /* framing errors are soft errors */
                    else if ((flags & NV_RX_ERROR_MASK) == NV_RX_FRAMINGERR) {
                        if (flags & NV_RX_SUBSTRACT1) {
                            len--;
                        }
                    }
                    /* the rest are hard errors */
                    else {
                        if (flags & NV_RX_MISSEDFRAME) {
                            //dev->stats.rx_missed_errors++;
                        }
                        if (flags & NV_RX_CRCERR) {
                            //dev->stats.rx_crc_errors++;
                        }
                        if (flags & NV_RX_OVERFLOW) {
                            //dev->stats.rx_over_errors++;
                        }
                        //dev->stats.rx_errors++;
                        delete_packet(skb);
                        goto next_pkt;
                    }
                }
            } else {
                delete_packet(skb);
                goto next_pkt;
            }
        } else {
            if (__likely(flags & NV_RX2_DESCRIPTORVALID)) {
                len = flags & LEN_MASK_V2;
                if (__unlikely(flags & NV_RX2_ERROR)) {
                    if ((flags & NV_RX2_ERROR_MASK) == NV_RX2_ERROR4) {
                        len = nv_getlen(dev, skb->data, len);
                        if (len < 0) {
                            //dev->stats.rx_errors++;
                            delete_packet(skb);
                            goto next_pkt;
                        }
                    }
                    /* framing errors are soft errors */
                    else if ((flags & NV_RX2_ERROR_MASK) == NV_RX2_FRAMINGERR) {
                        if (flags & NV_RX2_SUBSTRACT1) {
                            len--;
                        }
                    }
                    /* the rest are hard errors */
                    else {
                        if (flags & NV_RX2_CRCERR) {
                            //dev->stats.rx_crc_errors++;
                        }
                        if (flags & NV_RX2_OVERFLOW) {
                            //dev->stats.rx_over_errors++;
                        }
                        //dev->stats.rx_errors++;
                        delete_packet(skb);
                        goto next_pkt;
                    }
                }
                if (((flags & NV_RX2_CHECKSUMMASK) == NV_RX2_CHECKSUM_IP_TCP) || /*ip and tcp */
                    ((flags & NV_RX2_CHECKSUMMASK) == NV_RX2_CHECKSUM_IP_UDP)) { /*ip and udp */
                    // todo giszo skb->ip_summed = CHECKSUM_UNNECESSARY;
                }
            } else {
                delete_packet(skb);
                goto next_pkt;
            }
        }

        /* got a valid packet - forward it to the network core */
#if 0
        skb_put(skb, len);
        skb->protocol = eth_type_trans(skb, dev);
        dprintk(KERN_DEBUG "%s: nv_rx_process: %d bytes, proto %d accepted.\n",
                    dev->name, len, skb->protocol);
        netif_rx(skb);

        dev->stats.rx_packets++;
        dev->stats.rx_bytes += len;
#endif

next_pkt:
        if (__unlikely(np->get_rx.orig++ == np->last_rx.orig))
            np->get_rx.orig = np->first_rx.orig;
        if (__unlikely(np->get_rx_ctx++ == np->last_rx_ctx))
            np->get_rx_ctx = np->first_rx_ctx;

        rx_work++;
    }

    return rx_work;
}

static int nv_rx_process_optimized(struct net_device *dev, int limit)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint32_t flags;
    int rx_work = 0;
    packet_t* skb;
    int len;

    while((np->get_rx.ex != np->put_rx.ex) &&
          !((flags = np->get_rx.ex->flaglen) & NV_RX2_AVAIL) &&
          (rx_work < limit)) {

        DEBUG_LOG( "nvidia: nv_rx_process_optimized: flags 0x%x.\n",flags);

        /*
         * the packet is for us.
         * TODO: check if a prefetch of the first cacheline improves
         * the performance.
         */
        skb = np->get_rx_ctx->skb;
        np->get_rx_ctx->skb = NULL;

        /* look at what we actually got: */
        if (__likely(flags & NV_RX2_DESCRIPTORVALID)) {
            len = flags & LEN_MASK_V2;
            if (__unlikely(flags & NV_RX2_ERROR)) {
                if ((flags & NV_RX2_ERROR_MASK) == NV_RX2_ERROR4) {
                    len = nv_getlen(dev, skb->data, len);
                    if (len < 0) {
                        delete_packet(skb);
                        goto next_pkt;
                    }
                }
                /* framing errors are soft errors */
                else if ((flags & NV_RX2_ERROR_MASK) == NV_RX2_FRAMINGERR) {
                    if (flags & NV_RX2_SUBSTRACT1) {
                        len--;
                    }
                }
                /* the rest are hard errors */
                else {
                    delete_packet(skb);
                    goto next_pkt;
                }
            }

            if (((flags & NV_RX2_CHECKSUMMASK) == NV_RX2_CHECKSUM_IP_TCP) || /*ip and tcp */
                ((flags & NV_RX2_CHECKSUMMASK) == NV_RX2_CHECKSUM_IP_UDP)) { /*ip and udp */
                //skb->ip_summed = CHECKSUM_UNNECESSARY;
            }

#if 0
            /* got a valid packet - forward it to the network core */
            skb_put(skb, len);
            skb->protocol = eth_type_trans(skb, dev);
            prefetch(skb->data);

            dprintk(KERN_DEBUG "%s: nv_rx_process_optimized: %d bytes, proto %d accepted.\n",
                dev->name, len, skb->protocol);

            netif_rx(skb);

            dev->stats.rx_packets++;
            dev->stats.rx_bytes += len;
#endif
        } else {
            delete_packet(skb);
        }
next_pkt:
        if (__unlikely(np->get_rx.ex++ == np->last_rx.ex))
            np->get_rx.ex = np->first_rx.ex;
        if (__unlikely(np->get_rx_ctx++ == np->last_rx_ctx))
            np->get_rx_ctx = np->first_rx_ctx;

        rx_work++;
    }

    return rx_work;
}

static void set_bufsize(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);

    if (dev->mtu <= ETH_DATA_LEN)
        np->rx_buf_sz = ETH_DATA_LEN + NV_RX_HEADERS;
    else
        np->rx_buf_sz = dev->mtu + NV_RX_HEADERS;
}

#if 0
/*
 * nv_change_mtu: dev->change_mtu function
 * Called with dev_base_lock held for read.
 */
static int nv_change_mtu(struct net_device *dev, int new_mtu)
{
    struct fe_priv *np = net_device_get_private(dev);
    int old_mtu;

    if (new_mtu < 64 || new_mtu > np->pkt_limit)
        return -EINVAL;

    old_mtu = dev->mtu;
    dev->mtu = new_mtu;

    /* return early if the buffer sizes will not change */
    if (old_mtu <= ETH_DATA_LEN && new_mtu <= ETH_DATA_LEN)
        return 0;
    if (old_mtu == new_mtu)
        return 0;

    /* synchronized against open : rtnl_lock() held by caller */
    if (net_device_running(dev)) {
        uint8_t* base = get_hwbase(dev);
        /*
         * It seems that the nic preloads valid ring entries into an
         * internal buffer. The procedure for flushing everything is
         * guessed, there is probably a simpler approach.
         * Changing the MTU is a rare event, it shouldn't matter.
         */
        nv_disable_irq(dev);
        // todo giszo netif_tx_lock_bh(dev);
        // todo giszo netif_addr_lock(dev);
        spinlock(&np->lock);
        /* stop engines */
        nv_stop_rxtx(dev);
        nv_txrx_reset(dev);
        /* drain rx queue */
        nv_drain_rxtx(dev);
        /* reinit driver view of the rx queue */
        set_bufsize(dev);
        if (nv_init_ring(dev)) {
            if (!np->in_shutdown)
                timer_setup(&np->oom_kick, get_system_time() + OOM_REFILL);
        }
        /* reinit nic view of the rx queue */
        writel(np->rx_buf_sz, base + NvRegOffloadConfig);
        setup_hw_rings(dev, NV_SETUP_RX_RING | NV_SETUP_TX_RING);
        writel( ((np->rx_ring_size-1) << NVREG_RINGSZ_RXSHIFT) + ((np->tx_ring_size-1) << NVREG_RINGSZ_TXSHIFT),
            base + NvRegRingSizes);
        pci_push(base);
        writel(NVREG_TXRXCTL_KICK|np->txrxctl_bits, get_hwbase(dev) + NvRegTxRxControl);
        pci_push(base);

        /* restart rx engine */
        nv_start_rxtx(dev);
        spinunlock(&np->lock);
        // todo giszo netif_addr_unlock(dev);
        // todo giszo netif_tx_unlock_bh(dev);
        nv_enable_irq(dev);
    }

    return 0;
}
#endif

static void nv_copy_mac_to_hw(struct net_device *dev)
{
    uint8_t* base = get_hwbase(dev);
    uint32_t mac[2];

    mac[0] = (dev->dev_addr[0] << 0) + (dev->dev_addr[1] << 8) +
            (dev->dev_addr[2] << 16) + (dev->dev_addr[3] << 24);
    mac[1] = (dev->dev_addr[4] << 0) + (dev->dev_addr[5] << 8);

    writel(mac[0], base + NvRegMacAddrA);
    writel(mac[1], base + NvRegMacAddrB);
}

#if 0
/*
 * nv_set_mac_address: dev->set_mac_address function
 * Called with rtnl_lock() held.
 */
static int nv_set_mac_address(struct net_device *dev, void *addr)
{
    struct fe_priv *np = net_device_get_private(dev);
    struct sockaddr *macaddr = (struct sockaddr*)addr;

    if (!is_valid_ethernet_address( (uint8_t*)macaddr->sa_data)) {
        return -EADDRNOTAVAIL;
    }

    /* synchronized against open : rtnl_lock() held by caller */
    memcpy(dev->dev_addr, macaddr->sa_data, ETH_ADDR_LEN);

    if (netif_running(dev)) {
        // todo giszo netif_tx_lock_bh(dev);
        // todo giszo netif_addr_lock(dev);
        spinlock_disable(&np->lock);

        /* stop rx engine */
        nv_stop_rx(dev);

        /* set mac address */
        nv_copy_mac_to_hw(dev);

        /* restart rx engine */
        nv_start_rx(dev);
        spinunlock_enable(&np->lock);
        // todo giszo netif_addr_unlock(dev);
        // todo giszo netif_tx_unlock_bh(dev);
    } else {
        nv_copy_mac_to_hw(dev);
    }

    return 0;
}
#endif

static void nv_update_pause(struct net_device *dev, uint32_t pause_flags)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base = get_hwbase(dev);

    np->pause_flags &= ~(NV_PAUSEFRAME_TX_ENABLE | NV_PAUSEFRAME_RX_ENABLE);

    if (np->pause_flags & NV_PAUSEFRAME_RX_CAPABLE) {
        uint32_t pff = readl(base + NvRegPacketFilterFlags) & ~NVREG_PFF_PAUSE_RX;
        if (pause_flags & NV_PAUSEFRAME_RX_ENABLE) {
            writel(pff|NVREG_PFF_PAUSE_RX, base + NvRegPacketFilterFlags);
            np->pause_flags |= NV_PAUSEFRAME_RX_ENABLE;
        } else {
            writel(pff, base + NvRegPacketFilterFlags);
        }
    }
    if (np->pause_flags & NV_PAUSEFRAME_TX_CAPABLE) {
        uint32_t regmisc = readl(base + NvRegMisc1) & ~NVREG_MISC1_PAUSE_TX;
        if (pause_flags & NV_PAUSEFRAME_TX_ENABLE) {
            uint32_t pause_enable = NVREG_TX_PAUSEFRAME_ENABLE_V1;
            if (np->driver_data & DEV_HAS_PAUSEFRAME_TX_V2)
                pause_enable = NVREG_TX_PAUSEFRAME_ENABLE_V2;
            if (np->driver_data & DEV_HAS_PAUSEFRAME_TX_V3) {
                pause_enable = NVREG_TX_PAUSEFRAME_ENABLE_V3;
                /* limit the number of tx pause frames to a default of 8 */
                writel(readl(base + NvRegTxPauseFrameLimit)|NVREG_TX_PAUSEFRAMELIMIT_ENABLE, base + NvRegTxPauseFrameLimit);
            }
            writel(pause_enable,  base + NvRegTxPauseFrame);
            writel(regmisc|NVREG_MISC1_PAUSE_TX, base + NvRegMisc1);
            np->pause_flags |= NV_PAUSEFRAME_TX_ENABLE;
        } else {
            writel(NVREG_TX_PAUSEFRAME_DISABLE,  base + NvRegTxPauseFrame);
            writel(regmisc, base + NvRegMisc1);
        }
    }
}

/**
 * nv_update_linkspeed: Setup the MAC according to the link partner
 * @dev: Network device to be configured
 *
 * The function queries the PHY and checks if there is a link partner.
 * If yes, then it sets up the MAC accordingly. Otherwise, the MAC is
 * set to 10 MBit HD.
 *
 * The function returns 0 if there is no link partner and 1 if there is
 * a good link partner.
 */
static int nv_update_linkspeed(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base = get_hwbase(dev);
    int adv = 0;
    int lpa = 0;
    int adv_lpa, adv_pause, lpa_pause;
    int newls = np->linkspeed;
    int newdup = np->duplex;
    int mii_status;
    int retval = 0;
    uint32_t control_1000, status_1000, phyreg, pause_flags, txreg;
    uint32_t txrxFlags = 0;
    uint32_t phy_exp;

    /* BMSR_LSTATUS is latched, read it twice:
     * we want the current value.
     */
    mii_rw(dev, np->phyaddr, MII_BMSR, MII_READ);
    mii_status = mii_rw(dev, np->phyaddr, MII_BMSR, MII_READ);

    if (!(mii_status & BMSR_LSTATUS)) {
        DEBUG_LOG( "nvidia: no link detected by phy - falling back to 10HD.\n");
        newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
        newdup = 0;
        retval = 0;
        goto set_speed;
    }

    if (np->autoneg == 0) {
        DEBUG_LOG( "nvidia: nv_update_linkspeed: autoneg off, PHY set to 0x%04x.\n", np->fixed_mode);
        if (np->fixed_mode & LPA_100FULL) {
            newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_100;
            newdup = 1;
        } else if (np->fixed_mode & LPA_100HALF) {
            newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_100;
            newdup = 0;
        } else if (np->fixed_mode & LPA_10FULL) {
            newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
            newdup = 1;
        } else {
            newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
            newdup = 0;
        }
        retval = 1;
        goto set_speed;
    }
    /* check auto negotiation is complete */
    if (!(mii_status & BMSR_ANEGCOMPLETE)) {
        /* still in autonegotiation - configure nic for 10 MBit HD and wait. */
        newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
        newdup = 0;
        retval = 0;
        DEBUG_LOG( "nvidia: autoneg not completed - falling back to 10HD.\n" );
        goto set_speed;
    }

    adv = mii_rw(dev, np->phyaddr, MII_ADVERTISE, MII_READ);
    lpa = mii_rw(dev, np->phyaddr, MII_LPA, MII_READ);
    DEBUG_LOG( "nvidia: nv_update_linkspeed: PHY advertises 0x%04x, lpa 0x%04x.\n", adv, lpa);

    retval = 1;
    if (np->gigabit == PHY_GIGABIT) {
        control_1000 = mii_rw(dev, np->phyaddr, MII_CTRL1000, MII_READ);
        status_1000 = mii_rw(dev, np->phyaddr, MII_STAT1000, MII_READ);

        if ((control_1000 & ADVERTISE_1000FULL) &&
            (status_1000 & LPA_1000FULL)) {
            DEBUG_LOG( "nvidia: nv_update_linkspeed: GBit ethernet detected.\n" );
            newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_1000;
            newdup = 1;
            goto set_speed;
        }
    }

    /* FIXME: handle parallel detection properly */
    adv_lpa = lpa & adv;
    if (adv_lpa & LPA_100FULL) {
        newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_100;
        newdup = 1;
    } else if (adv_lpa & LPA_100HALF) {
        newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_100;
        newdup = 0;
    } else if (adv_lpa & LPA_10FULL) {
        newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
        newdup = 1;
    } else if (adv_lpa & LPA_10HALF) {
        newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
        newdup = 0;
    } else {
        DEBUG_LOG( "nvidia: bad ability %04x - falling back to 10HD.\n", adv_lpa);
        newls = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
        newdup = 0;
    }

set_speed:
    if (np->duplex == newdup && np->linkspeed == newls)
        return retval;

    kprintf( INFO, "nvidia: changing link setting from %d/%d to %d/%d.\n",np->linkspeed, np->duplex, newls, newdup);

    np->duplex = newdup;
    np->linkspeed = newls;

    /* The transmitter and receiver must be restarted for safe update */
    if (readl(base + NvRegTransmitterControl) & NVREG_XMITCTL_START) {
        txrxFlags |= NV_RESTART_TX;
        nv_stop_tx(dev);
    }
    if (readl(base + NvRegReceiverControl) & NVREG_RCVCTL_START) {
        txrxFlags |= NV_RESTART_RX;
        nv_stop_rx(dev);
    }

    if (np->gigabit == PHY_GIGABIT) {
        phyreg = readl(base + NvRegSlotTime);
        phyreg &= ~(0x3FF00);
        if (((np->linkspeed & 0xFFF) == NVREG_LINKSPEED_10) ||
            ((np->linkspeed & 0xFFF) == NVREG_LINKSPEED_100))
            phyreg |= NVREG_SLOTTIME_10_100_FULL;
        else if ((np->linkspeed & 0xFFF) == NVREG_LINKSPEED_1000)
            phyreg |= NVREG_SLOTTIME_1000_FULL;
        writel(phyreg, base + NvRegSlotTime);
    }

    phyreg = readl(base + NvRegPhyInterface);
    phyreg &= ~(PHY_HALF|PHY_100|PHY_1000);
    if (np->duplex == 0)
        phyreg |= PHY_HALF;
    if ((np->linkspeed & NVREG_LINKSPEED_MASK) == NVREG_LINKSPEED_100)
        phyreg |= PHY_100;
    else if ((np->linkspeed & NVREG_LINKSPEED_MASK) == NVREG_LINKSPEED_1000)
        phyreg |= PHY_1000;
    writel(phyreg, base + NvRegPhyInterface);

    phy_exp = mii_rw(dev, np->phyaddr, MII_EXPANSION, MII_READ) & EXPANSION_NWAY; /* autoneg capable */
    if (phyreg & PHY_RGMII) {
        if ((np->linkspeed & NVREG_LINKSPEED_MASK) == NVREG_LINKSPEED_1000) {
            txreg = NVREG_TX_DEFERRAL_RGMII_1000;
        } else {
            if (!phy_exp && !np->duplex && (np->driver_data & DEV_HAS_COLLISION_FIX)) {
                if ((np->linkspeed & NVREG_LINKSPEED_MASK) == NVREG_LINKSPEED_10)
                    txreg = NVREG_TX_DEFERRAL_RGMII_STRETCH_10;
                else
                    txreg = NVREG_TX_DEFERRAL_RGMII_STRETCH_100;
            } else {
                txreg = NVREG_TX_DEFERRAL_RGMII_10_100;
            }
        }
    } else {
        if (!phy_exp && !np->duplex && (np->driver_data & DEV_HAS_COLLISION_FIX))
            txreg = NVREG_TX_DEFERRAL_MII_STRETCH;
        else
            txreg = NVREG_TX_DEFERRAL_DEFAULT;
    }
    writel(txreg, base + NvRegTxDeferral);

    if (np->desc_ver == DESC_VER_1) {
        txreg = NVREG_TX_WM_DESC1_DEFAULT;
    } else {
        if ((np->linkspeed & NVREG_LINKSPEED_MASK) == NVREG_LINKSPEED_1000)
            txreg = NVREG_TX_WM_DESC2_3_1000;
        else
            txreg = NVREG_TX_WM_DESC2_3_DEFAULT;
    }
    writel(txreg, base + NvRegTxWatermark);

    writel(NVREG_MISC1_FORCE | ( np->duplex ? 0 : NVREG_MISC1_HD),
        base + NvRegMisc1);
    pci_push(base);
    writel(np->linkspeed, base + NvRegLinkSpeed);
    pci_push(base);

    pause_flags = 0;
    /* setup pause frame */
    if (np->duplex != 0) {
        if (np->autoneg && np->pause_flags & NV_PAUSEFRAME_AUTONEG) {
            adv_pause = adv & (ADVERTISE_PAUSE_CAP| ADVERTISE_PAUSE_ASYM);
            lpa_pause = lpa & (LPA_PAUSE_CAP| LPA_PAUSE_ASYM);

            switch (adv_pause) {
            case ADVERTISE_PAUSE_CAP:
                if (lpa_pause & LPA_PAUSE_CAP) {
                    pause_flags |= NV_PAUSEFRAME_RX_ENABLE;
                    if (np->pause_flags & NV_PAUSEFRAME_TX_REQ)
                        pause_flags |= NV_PAUSEFRAME_TX_ENABLE;
                }
                break;
            case ADVERTISE_PAUSE_ASYM:
                if (lpa_pause == (LPA_PAUSE_CAP| LPA_PAUSE_ASYM))
                {
                    pause_flags |= NV_PAUSEFRAME_TX_ENABLE;
                }
                break;
            case ADVERTISE_PAUSE_CAP| ADVERTISE_PAUSE_ASYM:
                if (lpa_pause & LPA_PAUSE_CAP)
                {
                    pause_flags |=  NV_PAUSEFRAME_RX_ENABLE;
                    if (np->pause_flags & NV_PAUSEFRAME_TX_REQ)
                        pause_flags |= NV_PAUSEFRAME_TX_ENABLE;
                }
                if (lpa_pause == LPA_PAUSE_ASYM)
                {
                    pause_flags |= NV_PAUSEFRAME_RX_ENABLE;
                }
                break;
            }
        } else {
            pause_flags = np->pause_flags;
        }
    }
    nv_update_pause(dev, pause_flags);

    if (txrxFlags & NV_RESTART_TX)
        nv_start_tx(dev);
    if (txrxFlags & NV_RESTART_RX)
        nv_start_rx(dev);

    return retval;
}

static void nv_linkchange(struct net_device *dev)
{
    if (nv_update_linkspeed(dev)) {
        if (!net_device_carrier_ok(dev)) {
            net_device_carrier_on(dev);
            kprintf( INFO, "nvidia: link up.\n" );
            nv_txrx_gate(dev, false);
            nv_start_rx(dev);
        }
    } else {
        if (net_device_carrier_ok(dev)) {
            net_device_carrier_off(dev);
            kprintf( INFO, "nvidia: link down.\n" );
            nv_txrx_gate(dev, true);
            nv_stop_rx(dev);
        }
    }
}

static void nv_link_irq(struct net_device *dev)
{
    uint8_t* base = get_hwbase(dev);
    uint32_t miistat;

    miistat = readl(base + NvRegMIIStatus);
    writel(NVREG_MIISTAT_LINKCHANGE, base + NvRegMIIStatus);
    kprintf( INFO, "nvidia: link change irq, status 0x%x.\n", miistat);

    if (miistat & (NVREG_MIISTAT_LINKCHANGE))
        nv_linkchange(dev);
}

static inline int nv_change_interrupt_mode(struct net_device *dev, int total_work)
{
    struct fe_priv *np = net_device_get_private(dev);

    if (optimization_mode == NV_OPTIMIZATION_MODE_DYNAMIC) {
        if (total_work > NV_DYNAMIC_THRESHOLD) {
            /* transition to poll based interrupts */
            np->quiet_count = 0;
            if (np->irqmask != NVREG_IRQMASK_CPU) {
                np->irqmask = NVREG_IRQMASK_CPU;
                return 1;
            }
        } else {
            if (np->quiet_count < NV_DYNAMIC_MAX_QUIET_COUNT) {
                np->quiet_count++;
            } else {
                /* reached a period of low activity, switch
                   to per tx/rx packet interrupts */
                if (np->irqmask != NVREG_IRQMASK_THROUGHPUT) {
                    np->irqmask = NVREG_IRQMASK_THROUGHPUT;
                    return 1;
                }
            }
        }
    }
    return 0;
}

static int nv_nic_irq(int irq, void *data, registers_t* regs)
{
    struct net_device *dev = (struct net_device *) data;
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base = get_hwbase(dev);
    int total_work = 0;
    int loop_count = 0;

    np->events = readl(base + NvRegIrqStatus);
    writel(np->events, base + NvRegIrqStatus);

    if (!(np->events & np->irqmask))
        return 0;

    do
    {
        int work = 0;
        if ((work = nv_rx_process(dev, RX_WORK_PER_LOOP))) {
            if (__unlikely(nv_alloc_rx(dev))) {
                spinlock(&np->lock);
                if (!np->in_shutdown)
                    timer_setup(&np->oom_kick, get_system_time() + OOM_REFILL);
                spinunlock(&np->lock);
            }
        }

        spinlock(&np->lock);
        work += nv_tx_done(dev, TX_WORK_PER_LOOP);
        spinunlock(&np->lock);

        if (!work)
            break;

        total_work += work;

        loop_count++;
    } while (loop_count < max_interrupt_work);

    if (nv_change_interrupt_mode(dev, total_work)) {
        /* setup new irq mask */
        writel(np->irqmask, base + NvRegIrqMask);
    }

    if (__unlikely(np->events & NVREG_IRQ_LINK)) {
        spinlock(&np->lock);
        nv_link_irq(dev);
        spinunlock(&np->lock);
    }
    if (__unlikely(np->need_linktimer && ( get_system_time() > np->link_timeout ))) {
        spinlock(&np->lock);
        nv_linkchange(dev);
        spinunlock(&np->lock);
        np->link_timeout = get_system_time() + LINK_TIMEOUT;
    }
    if (__unlikely(np->events & NVREG_IRQ_RECOVER_ERROR)) {
        spinlock(&np->lock);
        /* disable interrupts on the nic */
        writel(0, base + NvRegIrqMask);
        pci_push(base);

        if (!np->in_shutdown) {
            np->nic_poll_irq = np->irqmask;
            np->recover_error = 1;
            timer_setup(&np->nic_poll, get_system_time() + POLL_WAIT);
        }
        spinunlock(&np->lock);
    }

    return 0;
}

/**
 * All _optimized functions are used to help increase performance
 * (reduce CPU and increase throughput). They use descripter version 3,
 * compiler directives, and reduce memory accesses.
 */
static int nv_nic_irq_optimized(int irq, void *data, registers_t* regs)
{
    struct net_device *dev = (struct net_device *) data;
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base = get_hwbase(dev);
    int total_work = 0;
    int loop_count = 0;

    np->events = readl(base + NvRegIrqStatus);
    writel(np->events, base + NvRegIrqStatus);

    if (!(np->events & np->irqmask))
        return 0;

    do
    {
        int work = 0;
        if ((work = nv_rx_process_optimized(dev, RX_WORK_PER_LOOP))) {
            if (__unlikely(nv_alloc_rx_optimized(dev))) {
                spinlock(&np->lock);
                if (!np->in_shutdown)
                    timer_setup(&np->oom_kick, get_system_time() + OOM_REFILL);
                spinunlock(&np->lock);
            }
        }

        spinlock(&np->lock);
        work += nv_tx_done_optimized(dev, TX_WORK_PER_LOOP);
        spinunlock(&np->lock);

        if (!work)
            break;

        total_work += work;

        loop_count++;
    }
    while (loop_count < max_interrupt_work);

    if (nv_change_interrupt_mode(dev, total_work)) {
        /* setup new irq mask */
        writel(np->irqmask, base + NvRegIrqMask);
    }

    if (__unlikely(np->events & NVREG_IRQ_LINK)) {
        spinlock(&np->lock);
        nv_link_irq(dev);
        spinunlock(&np->lock);
    }
    if (__unlikely(np->need_linktimer && ( get_system_time() > np->link_timeout) )) {
        spinlock(&np->lock);
        nv_linkchange(dev);
        spinunlock(&np->lock);
        np->link_timeout = get_system_time() + LINK_TIMEOUT;
    }
    if (__unlikely(np->events & NVREG_IRQ_RECOVER_ERROR)) {
        spinlock(&np->lock);
        /* disable interrupts on the nic */
        writel(0, base + NvRegIrqMask);
        pci_push(base);

        if (!np->in_shutdown) {
            np->nic_poll_irq = np->irqmask;
            np->recover_error = 1;
            timer_setup(&np->nic_poll, get_system_time() + POLL_WAIT);
        }
        spinunlock(&np->lock);
    }

    return 0;
}

static int nv_nic_irq_test(int irq, void *data, registers_t* regs)
{
    struct net_device *dev = (struct net_device *) data;
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base = get_hwbase(dev);
    uint32_t events;

    events = readl(base + NvRegIrqStatus) & NVREG_IRQSTAT_MASK;
    writel(NVREG_IRQ_TIMER, base + NvRegIrqStatus);

    pci_push(base);

    if (!(events & NVREG_IRQ_TIMER))
        return 0;

    spinlock(&np->lock);
    np->intr_test = 1;
    spinunlock(&np->lock);

    return 0;
}

static int nv_request_irq(struct net_device *dev, int intr_test)
{
    struct fe_priv *np = get_nvpriv(dev);
    irq_handler_t* handler;

    if (intr_test) {
        handler = nv_nic_irq_test;
    } else {
        if (nv_optimized(np))
            handler = nv_nic_irq_optimized;
        else
            handler = nv_nic_irq;
    }

    if (request_irq(np->dev_irq, handler, dev) != 0) {
        goto out_err;
    }

    return 0;

out_err:
    return 1;
}

static void nv_free_irq(struct net_device *dev)
{
    struct fe_priv *np = get_nvpriv(dev);

    release_irq_all(np->dev_irq);
}

static int nv_do_nic_poll(void* data)
{
    struct net_device *dev = (struct net_device *) data;
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base = get_hwbase(dev);
    uint32_t mask = 0;

    /*
     * First disable irq(s) and then
     * reenable interrupts on the nic, we have to do this before calling
     * nv_nic_irq because that may decide to do otherwise
     */

    disable_irq(np->dev_irq);

    mask = np->irqmask;

    /* disable_irq() contains synchronize_irq, thus no irq handler can run now */

    if (np->recover_error) {
        np->recover_error = 0;
        kprintf( INFO, "nvidia: MAC in recoverable error state\n");
        if (net_device_running(dev)) {
            // netif_tx_lock_bh(dev);
            //netif_addr_lock(dev);
            spinlock(&np->lock);
            /* stop engines */
            nv_stop_rxtx(dev);
            if (np->driver_data & DEV_HAS_POWER_CNTRL)
                nv_mac_reset(dev);
            nv_txrx_reset(dev);
            /* drain rx queue */
            nv_drain_rxtx(dev);
            /* reinit driver view of the rx queue */
            set_bufsize(dev);
            if (nv_init_ring(dev)) {
                if (!np->in_shutdown)
                    timer_setup(&np->oom_kick, get_system_time() + OOM_REFILL);
            }
            /* reinit nic view of the rx queue */
            writel(np->rx_buf_sz, base + NvRegOffloadConfig);
            setup_hw_rings(dev, NV_SETUP_RX_RING | NV_SETUP_TX_RING);
            writel( ((np->rx_ring_size-1) << NVREG_RINGSZ_RXSHIFT) + ((np->tx_ring_size-1) << NVREG_RINGSZ_TXSHIFT),
                base + NvRegRingSizes);
            pci_push(base);
            writel(NVREG_TXRXCTL_KICK|np->txrxctl_bits, get_hwbase(dev) + NvRegTxRxControl);
            pci_push(base);
            /* clear interrupts */
            writel(NVREG_IRQSTAT_MASK, base + NvRegIrqStatus);

            /* restart rx engine */
            nv_start_rxtx(dev);
            spinunlock(&np->lock);
            //netif_addr_unlock(dev);
            //netif_tx_unlock_bh(dev);
        }
    }

    writel(mask, base + NvRegIrqMask);
    pci_push(base);

    np->nic_poll_irq = 0;

    if (nv_optimized(np))
        nv_nic_irq_optimized(0, dev, NULL);
    else
        nv_nic_irq(0, dev, NULL);

    enable_irq(np->dev_irq);

    return 0;
}

static int nv_do_stats_poll(void* data)
{
    struct net_device *dev = (struct net_device *) data;
    struct fe_priv *np = net_device_get_private(dev);

    nv_get_hw_stats(dev);

    if (!np->in_shutdown)
        timer_setup(&np->stats_poll, get_system_time() + STATS_INTERVAL);

    return 0;
}

#if 0
static int nv_get_settings(struct net_device *dev, struct ethtool_cmd *ecmd)
{
    struct fe_priv *np = net_device_get_private(dev);
    int adv;

    spinlock_disable(&np->lock);
    ecmd->port = PORT_MII;
    if (!netif_running(dev)) {
        /* We do not track link speed / duplex setting if the
         * interface is disabled. Force a link check */
        if (nv_update_linkspeed(dev)) {
            if (!net_device_carrier_ok(dev))
                net_device_carrier_on(dev);
        } else {
            if (net_device_carrier_ok(dev))
                net_device_carrier_off(dev);
        }
    }

    if (netif_carrier_ok(dev)) {
        switch(np->linkspeed & (NVREG_LINKSPEED_MASK)) {
        case NVREG_LINKSPEED_10:
            ecmd->speed = SPEED_10;
            break;
        case NVREG_LINKSPEED_100:
            ecmd->speed = SPEED_100;
            break;
        case NVREG_LINKSPEED_1000:
            ecmd->speed = SPEED_1000;
            break;
        }
        ecmd->duplex = DUPLEX_HALF;
        if (np->duplex)
            ecmd->duplex = DUPLEX_FULL;
    } else {
        ecmd->speed = -1;
        ecmd->duplex = -1;
    }

    ecmd->autoneg = np->autoneg;

    ecmd->advertising = ADVERTISED_MII;
    if (np->autoneg) {
        ecmd->advertising |= ADVERTISED_Autoneg;
        adv = mii_rw(dev, np->phyaddr, MII_ADVERTISE, MII_READ);
        if (adv & ADVERTISE_10HALF)
            ecmd->advertising |= ADVERTISED_10baseT_Half;
        if (adv & ADVERTISE_10FULL)
            ecmd->advertising |= ADVERTISED_10baseT_Full;
        if (adv & ADVERTISE_100HALF)
            ecmd->advertising |= ADVERTISED_100baseT_Half;
        if (adv & ADVERTISE_100FULL)
            ecmd->advertising |= ADVERTISED_100baseT_Full;
        if (np->gigabit == PHY_GIGABIT) {
            adv = mii_rw(dev, np->phyaddr, MII_CTRL1000, MII_READ);
            if (adv & ADVERTISE_1000FULL)
                ecmd->advertising |= ADVERTISED_1000baseT_Full;
        }
    }
    ecmd->supported = (SUPPORTED_Autoneg |
        SUPPORTED_10baseT_Half | SUPPORTED_10baseT_Full |
        SUPPORTED_100baseT_Half | SUPPORTED_100baseT_Full |
        SUPPORTED_MII);
    if (np->gigabit == PHY_GIGABIT)
        ecmd->supported |= SUPPORTED_1000baseT_Full;

    ecmd->phy_address = np->phyaddr;
    ecmd->transceiver = XCVR_EXTERNAL;

    /* ignore maxtxpkt, maxrxpkt for now */
    spinunlock_enable(&np->lock);
    return 0;
}

static int nv_set_settings(struct net_device *dev, struct ethtool_cmd *ecmd)
{
    struct fe_priv *np = net_device_get_private(dev);

    if (ecmd->port != PORT_MII)
        return -EINVAL;
    if (ecmd->transceiver != XCVR_EXTERNAL)
        return -EINVAL;
    if (ecmd->phy_address != np->phyaddr) {
        /* TODO: support switching between multiple phys. Should be
         * trivial, but not enabled due to lack of test hardware. */
        return -EINVAL;
    }
    if (ecmd->autoneg == AUTONEG_ENABLE) {
        uint32_t mask;

        mask = ADVERTISED_10baseT_Half | ADVERTISED_10baseT_Full |
              ADVERTISED_100baseT_Half | ADVERTISED_100baseT_Full;
        if (np->gigabit == PHY_GIGABIT)
            mask |= ADVERTISED_1000baseT_Full;

        if ((ecmd->advertising & mask) == 0)
            return -EINVAL;

    } else if (ecmd->autoneg == AUTONEG_DISABLE) {
        /* Note: autonegotiation disable, speed 1000 intentionally
         * forbidden - noone should need that. */

        if (ecmd->speed != SPEED_10 && ecmd->speed != SPEED_100)
            return -EINVAL;
        if (ecmd->duplex != DUPLEX_HALF && ecmd->duplex != DUPLEX_FULL)
            return -EINVAL;
    } else {
        return -EINVAL;
    }

    netif_carrier_off(dev);
    if (netif_running(dev)) {
        unsigned long flags;

        nv_disable_irq(dev);
        netif_tx_lock_bh(dev);
        netif_addr_lock(dev);
        /* with plain spinlock lockdep complains */
        spinlock_disablesave(&np->lock, flags);
        /* stop engines */
        /* FIXME:
         * this can take some time, and interrupts are disabled
         * due to spinlock_disablesave, but let's hope no daemon
         * is going to change the settings very often...
         * Worst case:
         * NV_RXSTOP_DELAY1MAX + NV_TXSTOP_DELAY1MAX
         * + some minor delays, which is up to a second approximately
         */
        nv_stop_rxtx(dev);
        spinunlock_enablerestore(&np->lock, flags);
        netif_addr_unlock(dev);
        netif_tx_unlock_bh(dev);
    }

    if (ecmd->autoneg == AUTONEG_ENABLE) {
        int adv, bmcr;

        np->autoneg = 1;

        /* advertise only what has been requested */
        adv = mii_rw(dev, np->phyaddr, MII_ADVERTISE, MII_READ);
        adv &= ~(ADVERTISE_ALL | ADVERTISE_100BASE4 | ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM);
        if (ecmd->advertising & ADVERTISED_10baseT_Half)
            adv |= ADVERTISE_10HALF;
        if (ecmd->advertising & ADVERTISED_10baseT_Full)
            adv |= ADVERTISE_10FULL;
        if (ecmd->advertising & ADVERTISED_100baseT_Half)
            adv |= ADVERTISE_100HALF;
        if (ecmd->advertising & ADVERTISED_100baseT_Full)
            adv |= ADVERTISE_100FULL;
        if (np->pause_flags & NV_PAUSEFRAME_RX_REQ)  /* for rx we set both advertisments but disable tx pause */
            adv |=  ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM;
        if (np->pause_flags & NV_PAUSEFRAME_TX_REQ)
            adv |=  ADVERTISE_PAUSE_ASYM;
        mii_rw(dev, np->phyaddr, MII_ADVERTISE, adv);

        if (np->gigabit == PHY_GIGABIT) {
            adv = mii_rw(dev, np->phyaddr, MII_CTRL1000, MII_READ);
            adv &= ~ADVERTISE_1000FULL;
            if (ecmd->advertising & ADVERTISED_1000baseT_Full)
                adv |= ADVERTISE_1000FULL;
            mii_rw(dev, np->phyaddr, MII_CTRL1000, adv);
        }

        if (netif_running(dev))
            kprintf( INFO, "%s: link down.\n", dev->name);
        bmcr = mii_rw(dev, np->phyaddr, MII_BMCR, MII_READ);
        if (np->phy_model == PHY_MODEL_MARVELL_E3016) {
            bmcr |= BMCR_ANENABLE;
            /* reset the phy in order for settings to stick,
             * and cause autoneg to start */
            if (phy_reset(dev, bmcr)) {
                kprintf( INFO, "%s: phy reset failed\n", dev->name);
                return -EINVAL;
            }
        } else {
            bmcr |= (BMCR_ANENABLE | BMCR_ANRESTART);
            mii_rw(dev, np->phyaddr, MII_BMCR, bmcr);
        }
    } else {
        int adv, bmcr;

        np->autoneg = 0;

        adv = mii_rw(dev, np->phyaddr, MII_ADVERTISE, MII_READ);
        adv &= ~(ADVERTISE_ALL | ADVERTISE_100BASE4 | ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM);
        if (ecmd->speed == SPEED_10 && ecmd->duplex == DUPLEX_HALF)
            adv |= ADVERTISE_10HALF;
        if (ecmd->speed == SPEED_10 && ecmd->duplex == DUPLEX_FULL)
            adv |= ADVERTISE_10FULL;
        if (ecmd->speed == SPEED_100 && ecmd->duplex == DUPLEX_HALF)
            adv |= ADVERTISE_100HALF;
        if (ecmd->speed == SPEED_100 && ecmd->duplex == DUPLEX_FULL)
            adv |= ADVERTISE_100FULL;
        np->pause_flags &= ~(NV_PAUSEFRAME_AUTONEG|NV_PAUSEFRAME_RX_ENABLE|NV_PAUSEFRAME_TX_ENABLE);
        if (np->pause_flags & NV_PAUSEFRAME_RX_REQ) {/* for rx we set both advertisments but disable tx pause */
            adv |=  ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM;
            np->pause_flags |= NV_PAUSEFRAME_RX_ENABLE;
        }
        if (np->pause_flags & NV_PAUSEFRAME_TX_REQ) {
            adv |=  ADVERTISE_PAUSE_ASYM;
            np->pause_flags |= NV_PAUSEFRAME_TX_ENABLE;
        }
        mii_rw(dev, np->phyaddr, MII_ADVERTISE, adv);
        np->fixed_mode = adv;

        if (np->gigabit == PHY_GIGABIT) {
            adv = mii_rw(dev, np->phyaddr, MII_CTRL1000, MII_READ);
            adv &= ~ADVERTISE_1000FULL;
            mii_rw(dev, np->phyaddr, MII_CTRL1000, adv);
        }

        bmcr = mii_rw(dev, np->phyaddr, MII_BMCR, MII_READ);
        bmcr &= ~(BMCR_ANENABLE|BMCR_SPEED100|BMCR_SPEED1000|BMCR_FULLDPLX);
        if (np->fixed_mode & (ADVERTISE_10FULL|ADVERTISE_100FULL))
            bmcr |= BMCR_FULLDPLX;
        if (np->fixed_mode & (ADVERTISE_100HALF|ADVERTISE_100FULL))
            bmcr |= BMCR_SPEED100;
        if (np->phy_oui == PHY_OUI_MARVELL) {
            /* reset the phy in order for forced mode settings to stick */
            if (phy_reset(dev, bmcr)) {
                kprintf( INFO, "%s: phy reset failed\n", dev->name);
                return -EINVAL;
            }
        } else {
            mii_rw(dev, np->phyaddr, MII_BMCR, bmcr);
            if (netif_running(dev)) {
                /* Wait a bit and then reconfigure the nic. */
                udelay(10);
                nv_linkchange(dev);
            }
        }
    }

    if (netif_running(dev)) {
        nv_start_rxtx(dev);
        nv_enable_irq(dev);
    }

    return 0;
}

#define FORCEDETH_REGS_VER  1

static int nv_get_regs_len(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    return np->register_size;
}

static void nv_get_regs(struct net_device *dev, struct ethtool_regs *regs, void *buf)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base = get_hwbase(dev);
    uint32_t *rbuf = buf;
    int i;

    regs->version = FORCEDETH_REGS_VER;
    spinlock_disable(&np->lock);
    for (i = 0;i <= np->register_size/sizeof(uint32_t); i++)
        rbuf[i] = readl(base + i*sizeof(uint32_t));
    spinunlock_enable(&np->lock);
}

static int nv_nway_reset(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    int ret;

    if (np->autoneg) {
        int bmcr;

        netif_carrier_off(dev);
        if (netif_running(dev)) {
            nv_disable_irq(dev);
            netif_tx_lock_bh(dev);
            netif_addr_lock(dev);
            spin_lock(&np->lock);
            /* stop engines */
            nv_stop_rxtx(dev);
            spin_unlock(&np->lock);
            netif_addr_unlock(dev);
            netif_tx_unlock_bh(dev);
            kprintf( INFO, "%s: link down.\n", dev->name);
        }

        bmcr = mii_rw(dev, np->phyaddr, MII_BMCR, MII_READ);
        if (np->phy_model == PHY_MODEL_MARVELL_E3016) {
            bmcr |= BMCR_ANENABLE;
            /* reset the phy in order for settings to stick*/
            if (phy_reset(dev, bmcr)) {
                kprintf( INFO, "%s: phy reset failed\n", dev->name);
                return -EINVAL;
            }
        } else {
            bmcr |= (BMCR_ANENABLE | BMCR_ANRESTART);
            mii_rw(dev, np->phyaddr, MII_BMCR, bmcr);
        }

        if (netif_running(dev)) {
            nv_start_rxtx(dev);
            nv_enable_irq(dev);
        }
        ret = 0;
    } else {
        ret = -EINVAL;
    }

    return ret;
}

static int nv_set_tso(struct net_device *dev, uint32_t value)
{
    struct fe_priv *np = net_device_get_private(dev);

    if ((np->driver_data & DEV_HAS_CHECKSUM))
        return ethtool_op_set_tso(dev, value);
    else
        return -EOPNOTSUPP;
}

static void nv_get_ringparam(struct net_device *dev, struct ethtool_ringparam* ring)
{
    struct fe_priv *np = net_device_get_private(dev);

    ring->rx_max_pending = (np->desc_ver == DESC_VER_1) ? RING_MAX_DESC_VER_1 : RING_MAX_DESC_VER_2_3;
    ring->rx_mini_max_pending = 0;
    ring->rx_jumbo_max_pending = 0;
    ring->tx_max_pending = (np->desc_ver == DESC_VER_1) ? RING_MAX_DESC_VER_1 : RING_MAX_DESC_VER_2_3;

    ring->rx_pending = np->rx_ring_size;
    ring->rx_mini_pending = 0;
    ring->rx_jumbo_pending = 0;
    ring->tx_pending = np->tx_ring_size;
}

static int nv_set_ringparam(struct net_device *dev, struct ethtool_ringparam* ring)
{
    struct fe_priv *np = net_device_get_private(dev);
    u8 __iomem *base = get_hwbase(dev);
    u8 *rxtx_ring, *rx_skbuff, *tx_skbuff;
    dma_addr_t ring_addr;

    if (ring->rx_pending < RX_RING_MIN ||
        ring->tx_pending < TX_RING_MIN ||
        ring->rx_mini_pending != 0 ||
        ring->rx_jumbo_pending != 0 ||
        (np->desc_ver == DESC_VER_1 &&
         (ring->rx_pending > RING_MAX_DESC_VER_1 ||
          ring->tx_pending > RING_MAX_DESC_VER_1)) ||
        (np->desc_ver != DESC_VER_1 &&
         (ring->rx_pending > RING_MAX_DESC_VER_2_3 ||
          ring->tx_pending > RING_MAX_DESC_VER_2_3))) {
        return -EINVAL;
    }

    /* allocate new rings */
    if (!nv_optimized(np)) {
        rxtx_ring = pci_alloc_consistent(np->pci_dev,
                        sizeof(struct ring_desc) * (ring->rx_pending + ring->tx_pending),
                        &ring_addr);
    } else {
        rxtx_ring = pci_alloc_consistent(np->pci_dev,
                        sizeof(struct ring_desc_ex) * (ring->rx_pending + ring->tx_pending),
                        &ring_addr);
    }
    rx_skbuff = kmalloc(sizeof(struct nv_skb_map) * ring->rx_pending);
    tx_skbuff = kmalloc(sizeof(struct nv_skb_map) * ring->tx_pending);
    if (!rxtx_ring || !rx_skbuff || !tx_skbuff) {
        /* fall back to old rings */
        if (!nv_optimized(np)) {
            if (rxtx_ring)
                pci_free_consistent(np->pci_dev, sizeof(struct ring_desc) * (ring->rx_pending + ring->tx_pending),
                            rxtx_ring, ring_addr);
        } else {
            if (rxtx_ring)
                pci_free_consistent(np->pci_dev, sizeof(struct ring_desc_ex) * (ring->rx_pending + ring->tx_pending),
                            rxtx_ring, ring_addr);
        }
        if (rx_skbuff)
            kfree(rx_skbuff);
        if (tx_skbuff)
            kfree(tx_skbuff);
        goto exit;
    }

    if (netif_running(dev)) {
        nv_disable_irq(dev);
        nv_napi_disable(dev);
        netif_tx_lock_bh(dev);
        netif_addr_lock(dev);
        spin_lock(&np->lock);
        /* stop engines */
        nv_stop_rxtx(dev);
        nv_txrx_reset(dev);
        /* drain queues */
        nv_drain_rxtx(dev);
        /* delete queues */
        free_rings(dev);
    }

    /* set new values */
    np->rx_ring_size = ring->rx_pending;
    np->tx_ring_size = ring->tx_pending;

    if (!nv_optimized(np)) {
        np->rx_ring.orig = (struct ring_desc*)rxtx_ring;
        np->tx_ring.orig = &np->rx_ring.orig[np->rx_ring_size];
    } else {
        np->rx_ring.ex = (struct ring_desc_ex*)rxtx_ring;
        np->tx_ring.ex = &np->rx_ring.ex[np->rx_ring_size];
    }
    np->rx_skb = (struct nv_skb_map*)rx_skbuff;
    np->tx_skb = (struct nv_skb_map*)tx_skbuff;
    np->ring_addr = ring_addr;

    memset(np->rx_skb, 0, sizeof(struct nv_skb_map) * np->rx_ring_size);
    memset(np->tx_skb, 0, sizeof(struct nv_skb_map) * np->tx_ring_size);

    if (netif_running(dev)) {
        /* reinit driver view of the queues */
        set_bufsize(dev);
        if (nv_init_ring(dev)) {
            if (!np->in_shutdown)
                mod_timer(&np->oom_kick, jiffies + OOM_REFILL);
        }

        /* reinit nic view of the queues */
        writel(np->rx_buf_sz, base + NvRegOffloadConfig);
        setup_hw_rings(dev, NV_SETUP_RX_RING | NV_SETUP_TX_RING);
        writel( ((np->rx_ring_size-1) << NVREG_RINGSZ_RXSHIFT) + ((np->tx_ring_size-1) << NVREG_RINGSZ_TXSHIFT),
            base + NvRegRingSizes);
        pci_push(base);
        writel(NVREG_TXRXCTL_KICK|np->txrxctl_bits, get_hwbase(dev) + NvRegTxRxControl);
        pci_push(base);

        /* restart engines */
        nv_start_rxtx(dev);
        spin_unlock(&np->lock);
        netif_addr_unlock(dev);
        netif_tx_unlock_bh(dev);
        nv_napi_enable(dev);
        nv_enable_irq(dev);
    }
    return 0;
exit:
    return -ENOMEM;
}

static void nv_get_pauseparam(struct net_device *dev, struct ethtool_pauseparam* pause)
{
    struct fe_priv *np = net_device_get_private(dev);

    pause->autoneg = (np->pause_flags & NV_PAUSEFRAME_AUTONEG) != 0;
    pause->rx_pause = (np->pause_flags & NV_PAUSEFRAME_RX_ENABLE) != 0;
    pause->tx_pause = (np->pause_flags & NV_PAUSEFRAME_TX_ENABLE) != 0;
}

static int nv_set_pauseparam(struct net_device *dev, struct ethtool_pauseparam* pause)
{
    struct fe_priv *np = net_device_get_private(dev);
    int adv, bmcr;

    if ((!np->autoneg && np->duplex == 0) ||
        (np->autoneg && !pause->autoneg && np->duplex == 0)) {
        kprintf( INFO, "%s: can not set pause settings when forced link is in half duplex.\n",
               dev->name);
        return -EINVAL;
    }
    if (pause->tx_pause && !(np->pause_flags & NV_PAUSEFRAME_TX_CAPABLE)) {
        kprintf( INFO, "%s: hardware does not support tx pause frames.\n", dev->name);
        return -EINVAL;
    }

    netif_carrier_off(dev);
    if (netif_running(dev)) {
        nv_disable_irq(dev);
        netif_tx_lock_bh(dev);
        netif_addr_lock(dev);
        spin_lock(&np->lock);
        /* stop engines */
        nv_stop_rxtx(dev);
        spin_unlock(&np->lock);
        netif_addr_unlock(dev);
        netif_tx_unlock_bh(dev);
    }

    np->pause_flags &= ~(NV_PAUSEFRAME_RX_REQ|NV_PAUSEFRAME_TX_REQ);
    if (pause->rx_pause)
        np->pause_flags |= NV_PAUSEFRAME_RX_REQ;
    if (pause->tx_pause)
        np->pause_flags |= NV_PAUSEFRAME_TX_REQ;

    if (np->autoneg && pause->autoneg) {
        np->pause_flags |= NV_PAUSEFRAME_AUTONEG;

        adv = mii_rw(dev, np->phyaddr, MII_ADVERTISE, MII_READ);
        adv &= ~(ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM);
        if (np->pause_flags & NV_PAUSEFRAME_RX_REQ) /* for rx we set both advertisments but disable tx pause */
            adv |=  ADVERTISE_PAUSE_CAP | ADVERTISE_PAUSE_ASYM;
        if (np->pause_flags & NV_PAUSEFRAME_TX_REQ)
            adv |=  ADVERTISE_PAUSE_ASYM;
        mii_rw(dev, np->phyaddr, MII_ADVERTISE, adv);

        if (netif_running(dev))
            kprintf( INFO, "%s: link down.\n", dev->name);
        bmcr = mii_rw(dev, np->phyaddr, MII_BMCR, MII_READ);
        bmcr |= (BMCR_ANENABLE | BMCR_ANRESTART);
        mii_rw(dev, np->phyaddr, MII_BMCR, bmcr);
    } else {
        np->pause_flags &= ~(NV_PAUSEFRAME_AUTONEG|NV_PAUSEFRAME_RX_ENABLE|NV_PAUSEFRAME_TX_ENABLE);
        if (pause->rx_pause)
            np->pause_flags |= NV_PAUSEFRAME_RX_ENABLE;
        if (pause->tx_pause)
            np->pause_flags |= NV_PAUSEFRAME_TX_ENABLE;

        if (!netif_running(dev))
            nv_update_linkspeed(dev);
        else
            nv_update_pause(dev, np->pause_flags);
    }

    if (netif_running(dev)) {
        nv_start_rxtx(dev);
        nv_enable_irq(dev);
    }
    return 0;
}

static uint32_t nv_get_rx_csum(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    return (np->rx_csum) != 0;
}

static int nv_set_rx_csum(struct net_device *dev, uint32_t data)
{
    struct fe_priv *np = net_device_get_private(dev);
    u8 __iomem *base = get_hwbase(dev);
    int retcode = 0;

    if (np->driver_data & DEV_HAS_CHECKSUM) {
        if (data) {
            np->rx_csum = 1;
            np->txrxctl_bits |= NVREG_TXRXCTL_RXCHECK;
        } else {
            np->rx_csum = 0;
            /* vlan is dependent on rx checksum offload */
            if (!(np->vlanctl_bits & NVREG_VLANCONTROL_ENABLE))
                np->txrxctl_bits &= ~NVREG_TXRXCTL_RXCHECK;
        }
        if (netif_running(dev)) {
            spinlock_disable(&np->lock);
            writel(np->txrxctl_bits, base + NvRegTxRxControl);
            spinunlock_enable(&np->lock);
        }
    } else {
        return -EINVAL;
    }

    return retcode;
}

static int nv_set_tx_csum(struct net_device *dev, uint32_t data)
{
    struct fe_priv *np = net_device_get_private(dev);

    if (np->driver_data & DEV_HAS_CHECKSUM)
        return ethtool_op_set_tx_csum(dev, data);
    else
        return -EOPNOTSUPP;
}

static int nv_set_sg(struct net_device *dev, uint32_t data)
{
    struct fe_priv *np = net_device_get_private(dev);

    if (np->driver_data & DEV_HAS_CHECKSUM)
        return ethtool_op_set_sg(dev, data);
    else
        return -EOPNOTSUPP;
}

static int nv_get_sset_count(struct net_device *dev, int sset)
{
    struct fe_priv *np = net_device_get_private(dev);

    switch (sset) {
    case ETH_SS_TEST:
        if (np->driver_data & DEV_HAS_TEST_EXTENDED)
            return NV_TEST_COUNT_EXTENDED;
        else
            return NV_TEST_COUNT_BASE;
    case ETH_SS_STATS:
        if (np->driver_data & DEV_HAS_STATISTICS_V3)
            return NV_DEV_STATISTICS_V3_COUNT;
        else if (np->driver_data & DEV_HAS_STATISTICS_V2)
            return NV_DEV_STATISTICS_V2_COUNT;
        else if (np->driver_data & DEV_HAS_STATISTICS_V1)
            return NV_DEV_STATISTICS_V1_COUNT;
        else
            return 0;
    default:
        return -EOPNOTSUPP;
    }
}

static void nv_get_ethtool_stats(struct net_device *dev, struct ethtool_stats *estats, u64 *buffer)
{
    struct fe_priv *np = net_device_get_private(dev);

    /* update stats */
    nv_do_stats_poll((unsigned long)dev);

    memcpy(buffer, &np->estats, nv_get_sset_count(dev, ETH_SS_STATS)*sizeof(u64));
}

static int nv_link_test(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    int mii_status;

    mii_rw(dev, np->phyaddr, MII_BMSR, MII_READ);
    mii_status = mii_rw(dev, np->phyaddr, MII_BMSR, MII_READ);

    /* check phy link status */
    if (!(mii_status & BMSR_LSTATUS))
        return 0;
    else
        return 1;
}

static int nv_register_test(struct net_device *dev)
{
    u8 __iomem *base = get_hwbase(dev);
    int i = 0;
    uint32_t orig_read, new_read;

    do {
        orig_read = readl(base + nv_registers_test[i].reg);

        /* xor with mask to toggle bits */
        orig_read ^= nv_registers_test[i].mask;

        writel(orig_read, base + nv_registers_test[i].reg);

        new_read = readl(base + nv_registers_test[i].reg);

        if ((new_read & nv_registers_test[i].mask) != (orig_read & nv_registers_test[i].mask))
            return 0;

        /* restore original value */
        orig_read ^= nv_registers_test[i].mask;
        writel(orig_read, base + nv_registers_test[i].reg);

    } while (nv_registers_test[++i].reg != 0);

    return 1;
}

static int nv_interrupt_test(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base = get_hwbase(dev);
    int ret = 1;
    int testcnt;
    uint32_t save_poll_interval = 0;

    if (netif_running(dev)) {
        /* free current irq */
        nv_free_irq(dev);
        save_poll_interval = readl(base+NvRegPollingInterval);
    }

    /* flag to test interrupt handler */
    np->intr_test = 0;

    /* setup test irq */
    if (nv_request_irq(dev, 1))
        return 0;

    /* setup timer interrupt */
    writel(NVREG_POLL_DEFAULT_CPU, base + NvRegPollingInterval);
    writel(NVREG_UNKSETUP6_VAL, base + NvRegUnknownSetupReg6);

    nv_enable_hw_interrupts(dev, NVREG_IRQ_TIMER);

    /* wait for at least one interrupt */
    thread_sleep(100 * 1000);

    spinlock_disable(&np->lock);

    /* flag should be set within ISR */
    testcnt = np->intr_test;
    if (!testcnt)
        ret = 2;

    nv_disable_hw_interrupts(dev, NVREG_IRQ_TIMER);
    writel(NVREG_IRQSTAT_MASK, base + NvRegIrqStatus);

    spinunlock_enable(&np->lock);

    nv_free_irq(dev);

    if (netif_running(dev)) {
        writel(save_poll_interval, base + NvRegPollingInterval);
        writel(NVREG_UNKSETUP6_VAL, base + NvRegUnknownSetupReg6);
        /* restore original irq */
        if (nv_request_irq(dev, 0))
            return 0;
    }

    return ret;
}

static int nv_loopback_test(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base = get_hwbase(dev);
    struct sk_buff *tx_skb, *rx_skb;
    ptr_t test_dma_addr;
    uint32_t tx_flags_extra = (np->desc_ver == DESC_VER_1 ? NV_TX_LASTPACKET : NV_TX2_LASTPACKET);
    uint32_t flags;
    int len, i, pkt_len;
    uint8_t *pkt_data;
    uint32_t filter_flags = 0;
    uint32_t misc1_flags = 0;
    int ret = 1;

    if (netif_running(dev)) {
        nv_disable_irq(dev);
        filter_flags = readl(base + NvRegPacketFilterFlags);
        misc1_flags = readl(base + NvRegMisc1);
    } else {
        nv_txrx_reset(dev);
    }

    /* reinit driver view of the rx queue */
    set_bufsize(dev);
    nv_init_ring(dev);

    /* setup hardware for loopback */
    writel(NVREG_MISC1_FORCE, base + NvRegMisc1);
    writel(NVREG_PFF_ALWAYS | NVREG_PFF_LOOPBACK, base + NvRegPacketFilterFlags);

    /* reinit nic view of the rx queue */
    writel(np->rx_buf_sz, base + NvRegOffloadConfig);
    setup_hw_rings(dev, NV_SETUP_RX_RING | NV_SETUP_TX_RING);
    writel( ((np->rx_ring_size-1) << NVREG_RINGSZ_RXSHIFT) + ((np->tx_ring_size-1) << NVREG_RINGSZ_TXSHIFT),
        base + NvRegRingSizes);
    pci_push(base);

    /* restart rx engine */
    nv_start_rxtx(dev);

    /* setup packet for tx */
    pkt_len = ETH_DATA_LEN;
    tx_skb = dev_alloc_skb(pkt_len);
    if (!tx_skb) {
        kprintf(ERROR, "nvidia: dev_alloc_skb() failed during loopback test\n");
        ret = 0;
        goto out;
    }
    test_dma_addr = pci_map_single(np->pci_dev, tx_skb->data,
                       skb_tailroom(tx_skb),
                       PCI_DMA_FROMDEVICE);
    pkt_data = skb_put(tx_skb, pkt_len);
    for (i = 0; i < pkt_len; i++)
        pkt_data[i] = (uint8_t)(i & 0xff);

    if (!nv_optimized(np)) {
        np->tx_ring.orig[0].buf = cpu_to_le32(test_dma_addr);
        np->tx_ring.orig[0].flaglen = cpu_to_le32((pkt_len-1) | np->tx_flags | tx_flags_extra);
    } else {
        np->tx_ring.ex[0].bufhigh = cpu_to_le32(dma_high(test_dma_addr));
        np->tx_ring.ex[0].buflow = cpu_to_le32(dma_low(test_dma_addr));
        np->tx_ring.ex[0].flaglen = cpu_to_le32((pkt_len-1) | np->tx_flags | tx_flags_extra);
    }
    writel(NVREG_TXRXCTL_KICK|np->txrxctl_bits, get_hwbase(dev) + NvRegTxRxControl);
    pci_push(get_hwbase(dev));

    msleep(500);

    /* check for rx of the packet */
    if (!nv_optimized(np)) {
        flags = np->rx_ring.orig[0].flaglen;
        len = nv_descr_getlength(&np->rx_ring.orig[0], np->desc_ver);

    } else {
        flags = np->rx_ring.ex[0].flaglen;
        len = nv_descr_getlength_ex(&np->rx_ring.ex[0], np->desc_ver);
    }

    if (flags & NV_RX_AVAIL) {
        ret = 0;
    } else if (np->desc_ver == DESC_VER_1) {
        if (flags & NV_RX_ERROR)
            ret = 0;
    } else {
        if (flags & NV_RX2_ERROR) {
            ret = 0;
        }
    }

    if (ret) {
        if (len != pkt_len) {
            ret = 0;
            DEBUG_LOG( "nvidia: loopback len mismatch %d vs %d\n",
                len, pkt_len);
        } else {
            rx_skb = np->rx_skb[0].skb;
            for (i = 0; i < pkt_len; i++) {
                if (rx_skb->data[i] != (u8)(i & 0xff)) {
                    ret = 0;
                    DEBUG_LOG( "nvidia: loopback pattern check failed on byte %d\n",i);
                    break;
                }
            }
        }
    } else {
        DEBUG_LOG( "nvidia: loopback - did not receive test packet\n");
    }

    pci_unmap_single(np->pci_dev, test_dma_addr,
               (skb_end_pointer(tx_skb) - tx_skb->data),
               PCI_DMA_TODEVICE);
    dev_kfree_skb_any(tx_skb);
 out:
    /* stop engines */
    nv_stop_rxtx(dev);
    nv_txrx_reset(dev);
    /* drain rx queue */
    nv_drain_rxtx(dev);

    if (netif_running(dev)) {
        writel(misc1_flags, base + NvRegMisc1);
        writel(filter_flags, base + NvRegPacketFilterFlags);
        nv_enable_irq(dev);
    }

    return ret;
}

static void nv_self_test(struct net_device *dev, struct ethtool_test *test, u64 *buffer)
{
    struct fe_priv *np = net_device_get_private(dev);
    u8 __iomem *base = get_hwbase(dev);
    int result;
    memset(buffer, 0, nv_get_sset_count(dev, ETH_SS_TEST)*sizeof(u64));

    if (!nv_link_test(dev)) {
        test->flags |= ETH_TEST_FL_FAILED;
        buffer[0] = 1;
    }

    if (test->flags & ETH_TEST_FL_OFFLINE) {
        if (netif_running(dev)) {
            netif_stop_queue(dev);
            nv_napi_disable(dev);
            netif_tx_lock_bh(dev);
            netif_addr_lock(dev);
            spinlock_disable(&np->lock);
            nv_disable_hw_interrupts(dev, np->irqmask);
            writel(NVREG_IRQSTAT_MASK, base + NvRegIrqStatus);
            /* stop engines */
            nv_stop_rxtx(dev);
            nv_txrx_reset(dev);
            /* drain rx queue */
            nv_drain_rxtx(dev);
            spinunlock_enable(&np->lock);
            netif_addr_unlock(dev);
            netif_tx_unlock_bh(dev);
        }

        if (!nv_register_test(dev)) {
            test->flags |= ETH_TEST_FL_FAILED;
            buffer[1] = 1;
        }

        result = nv_interrupt_test(dev);
        if (result != 1) {
            test->flags |= ETH_TEST_FL_FAILED;
            buffer[2] = 1;
        }
        if (result == 0) {
            /* bail out */
            return;
        }

        if (!nv_loopback_test(dev)) {
            test->flags |= ETH_TEST_FL_FAILED;
            buffer[3] = 1;
        }

        if (netif_running(dev)) {
            /* reinit driver view of the rx queue */
            set_bufsize(dev);
            if (nv_init_ring(dev)) {
                if (!np->in_shutdown)
                    mod_timer(&np->oom_kick, jiffies + OOM_REFILL);
            }
            /* reinit nic view of the rx queue */
            writel(np->rx_buf_sz, base + NvRegOffloadConfig);
            setup_hw_rings(dev, NV_SETUP_RX_RING | NV_SETUP_TX_RING);
            writel( ((np->rx_ring_size-1) << NVREG_RINGSZ_RXSHIFT) + ((np->tx_ring_size-1) << NVREG_RINGSZ_TXSHIFT),
                base + NvRegRingSizes);
            pci_push(base);
            writel(NVREG_TXRXCTL_KICK|np->txrxctl_bits, get_hwbase(dev) + NvRegTxRxControl);
            pci_push(base);
            /* restart rx engine */
            nv_start_rxtx(dev);
            netif_start_queue(dev);
            nv_napi_enable(dev);
            nv_enable_hw_interrupts(dev, np->irqmask);
        }
    }
}

static void nv_get_strings(struct net_device *dev, uint32_t stringset, u8 *buffer)
{
    switch (stringset) {
    case ETH_SS_STATS:
        memcpy(buffer, &nv_estats_str, nv_get_sset_count(dev, ETH_SS_STATS)*sizeof(struct nv_ethtool_str));
        break;
    case ETH_SS_TEST:
        memcpy(buffer, &nv_etests_str, nv_get_sset_count(dev, ETH_SS_TEST)*sizeof(struct nv_ethtool_str));
        break;
    }
}

static const struct ethtool_ops ops = {
    .get_drvinfo = nv_get_drvinfo,
    .get_link = ethtool_op_get_link,
    .get_wol = nv_get_wol,
    .set_wol = nv_set_wol,
    .get_settings = nv_get_settings,
    .set_settings = nv_set_settings,
    .get_regs_len = nv_get_regs_len,
    .get_regs = nv_get_regs,
    .nway_reset = nv_nway_reset,
    .set_tso = nv_set_tso,
    .get_ringparam = nv_get_ringparam,
    .set_ringparam = nv_set_ringparam,
    .get_pauseparam = nv_get_pauseparam,
    .set_pauseparam = nv_set_pauseparam,
    .get_rx_csum = nv_get_rx_csum,
    .set_rx_csum = nv_set_rx_csum,
    .set_tx_csum = nv_set_tx_csum,
    .set_sg = nv_set_sg,
    .get_strings = nv_get_strings,
    .get_ethtool_stats = nv_get_ethtool_stats,
    .get_sset_count = nv_get_sset_count,
    .self_test = nv_self_test,
};
#endif

/* The mgmt unit and driver use a semaphore to access the phy during init */
static int nv_mgmt_acquire_sema(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base = get_hwbase(dev);
    int i;
    uint32_t tx_ctrl, mgmt_sema;

    for (i = 0; i < 10; i++) {
        mgmt_sema = readl(base + NvRegTransmitterControl) & NVREG_XMITCTL_MGMT_SEMA_MASK;
        if (mgmt_sema == NVREG_XMITCTL_MGMT_SEMA_FREE)
            break;
        thread_sleep(500 * 1000);
    }

    if (mgmt_sema != NVREG_XMITCTL_MGMT_SEMA_FREE)
        return 0;

    for (i = 0; i < 2; i++) {
        tx_ctrl = readl(base + NvRegTransmitterControl);
        tx_ctrl |= NVREG_XMITCTL_HOST_SEMA_ACQ;
        writel(tx_ctrl, base + NvRegTransmitterControl);

        /* verify that semaphore was acquired */
        tx_ctrl = readl(base + NvRegTransmitterControl);
        if (((tx_ctrl & NVREG_XMITCTL_HOST_SEMA_MASK) == NVREG_XMITCTL_HOST_SEMA_ACQ) &&
            ((tx_ctrl & NVREG_XMITCTL_MGMT_SEMA_MASK) == NVREG_XMITCTL_MGMT_SEMA_FREE)) {
            np->mgmt_sema = 1;
            return 1;
        }
        else
            udelay(50);
    }

    return 0;
}

static void nv_mgmt_release_sema(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base = get_hwbase(dev);
    uint32_t tx_ctrl;

    if (np->driver_data & DEV_HAS_MGMT_UNIT) {
        if (np->mgmt_sema) {
            tx_ctrl = readl(base + NvRegTransmitterControl);
            tx_ctrl &= ~NVREG_XMITCTL_HOST_SEMA_ACQ;
            writel(tx_ctrl, base + NvRegTransmitterControl);
        }
    }
}


static int nv_mgmt_get_version(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t*base = get_hwbase(dev);
    uint32_t data_ready = readl(base + NvRegTransmitterControl);
    uint32_t data_ready2 = 0;
    uint64_t start;
    int ready = 0;

    writel(NVREG_MGMTUNITGETVERSION, base + NvRegMgmtUnitGetVersion);
    writel(data_ready ^ NVREG_XMITCTL_DATA_START, base + NvRegTransmitterControl);

    start = get_system_time();

    while ( get_system_time() < ( start + 5 * 1000 * 1000 ) ) {
        data_ready2 = readl(base + NvRegTransmitterControl);

        if ((data_ready & NVREG_XMITCTL_DATA_READY) != (data_ready2 & NVREG_XMITCTL_DATA_READY)) {
            ready = 1;
            break;
        }

        sched_preempt();
    }

    if (!ready || (data_ready2 & NVREG_XMITCTL_DATA_ERROR))
        return 0;

    np->mgmt_version = readl(base + NvRegMgmtUnitVersion) & NVREG_MGMTUNITVERSION;

    return 1;
}

static int nv_open(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base = get_hwbase(dev);
    int ret = 1;
    int oom, i;
    uint32_t low;

    /* power up phy */
    mii_rw(dev, np->phyaddr, MII_BMCR,
           mii_rw(dev, np->phyaddr, MII_BMCR, MII_READ) & ~BMCR_PDOWN);

    nv_txrx_gate(dev, false);
    /* erase previous misconfiguration */
    if (np->driver_data & DEV_HAS_POWER_CNTRL)
        nv_mac_reset(dev);
    writel(NVREG_MCASTADDRA_FORCE, base + NvRegMulticastAddrA);
    writel(0, base + NvRegMulticastAddrB);
    writel(NVREG_MCASTMASKA_NONE, base + NvRegMulticastMaskA);
    writel(NVREG_MCASTMASKB_NONE, base + NvRegMulticastMaskB);
    writel(0, base + NvRegPacketFilterFlags);

    writel(0, base + NvRegTransmitterControl);
    writel(0, base + NvRegReceiverControl);

    writel(0, base + NvRegAdapterControl);

    if (np->pause_flags & NV_PAUSEFRAME_TX_CAPABLE)
        writel(NVREG_TX_PAUSEFRAME_DISABLE,  base + NvRegTxPauseFrame);

    /* initialize descriptor rings */
    set_bufsize(dev);
    oom = nv_init_ring(dev);

    writel(0, base + NvRegLinkSpeed);
    writel(readl(base + NvRegTransmitPoll) & NVREG_TRANSMITPOLL_MAC_ADDR_REV, base + NvRegTransmitPoll);
    nv_txrx_reset(dev);
    writel(0, base + NvRegUnknownSetupReg6);

    np->in_shutdown = 0;

    /* give hw rings */
    setup_hw_rings(dev, NV_SETUP_RX_RING | NV_SETUP_TX_RING);
    writel( ((np->rx_ring_size-1) << NVREG_RINGSZ_RXSHIFT) + ((np->tx_ring_size-1) << NVREG_RINGSZ_TXSHIFT),
        base + NvRegRingSizes);

    writel(np->linkspeed, base + NvRegLinkSpeed);
    if (np->desc_ver == DESC_VER_1)
        writel(NVREG_TX_WM_DESC1_DEFAULT, base + NvRegTxWatermark);
    else
        writel(NVREG_TX_WM_DESC2_3_DEFAULT, base + NvRegTxWatermark);
    writel(np->txrxctl_bits, base + NvRegTxRxControl);
    writel(np->vlanctl_bits, base + NvRegVlanControl);
    pci_push(base);
    writel(NVREG_TXRXCTL_BIT1|np->txrxctl_bits, base + NvRegTxRxControl);
    reg_delay(dev, NvRegUnknownSetupReg5, NVREG_UNKSETUP5_BIT31, NVREG_UNKSETUP5_BIT31,
            NV_SETUP5_DELAY, NV_SETUP5_DELAYMAX,
            "nvidia: open: SetupReg5, Bit 31 remained off\n");

    writel(0, base + NvRegMIIMask);
    writel(NVREG_IRQSTAT_MASK, base + NvRegIrqStatus);
    writel(NVREG_MIISTAT_MASK_ALL, base + NvRegMIIStatus);

    writel(NVREG_MISC1_FORCE | NVREG_MISC1_HD, base + NvRegMisc1);
    writel(readl(base + NvRegTransmitterStatus), base + NvRegTransmitterStatus);
    writel(NVREG_PFF_ALWAYS, base + NvRegPacketFilterFlags);
    writel(np->rx_buf_sz, base + NvRegOffloadConfig);

    writel(readl(base + NvRegReceiverStatus), base + NvRegReceiverStatus);

    random_get_bytes((uint8_t*)&low, sizeof(low));
    low &= NVREG_SLOTTIME_MASK;
    if (np->desc_ver == DESC_VER_1) {
        writel(low|NVREG_SLOTTIME_DEFAULT, base + NvRegSlotTime);
    } else {
        if (!(np->driver_data & DEV_HAS_GEAR_MODE)) {
            /* setup legacy backoff */
            writel(NVREG_SLOTTIME_LEGBF_ENABLED|NVREG_SLOTTIME_10_100_FULL|low, base + NvRegSlotTime);
        } else {
            writel(NVREG_SLOTTIME_10_100_FULL, base + NvRegSlotTime);
            nv_gear_backoff_reseed(dev);
        }
    }
    writel(NVREG_TX_DEFERRAL_DEFAULT, base + NvRegTxDeferral);
    writel(NVREG_RX_DEFERRAL_DEFAULT, base + NvRegRxDeferral);
    if (poll_interval == -1) {
        if (optimization_mode == NV_OPTIMIZATION_MODE_THROUGHPUT)
            writel(NVREG_POLL_DEFAULT_THROUGHPUT, base + NvRegPollingInterval);
        else
            writel(NVREG_POLL_DEFAULT_CPU, base + NvRegPollingInterval);
    }
    else
        writel(poll_interval & 0xFFFF, base + NvRegPollingInterval);
    writel(NVREG_UNKSETUP6_VAL, base + NvRegUnknownSetupReg6);
    writel((np->phyaddr << NVREG_ADAPTCTL_PHYSHIFT)|NVREG_ADAPTCTL_PHYVALID|NVREG_ADAPTCTL_RUNNING,
            base + NvRegAdapterControl);
    writel(NVREG_MIISPEED_BIT8|NVREG_MIIDELAY, base + NvRegMIISpeed);
    writel(NVREG_MII_LINKCHANGE, base + NvRegMIIMask);
    if (np->wolenabled)
        writel(NVREG_WAKEUPFLAGS_ENABLE , base + NvRegWakeUpFlags);

    i = readl(base + NvRegPowerState);
    if ( (i & NVREG_POWERSTATE_POWEREDUP) == 0)
        writel(NVREG_POWERSTATE_POWEREDUP|i, base + NvRegPowerState);

    pci_push(base);
    udelay(10);
    writel(readl(base + NvRegPowerState) | NVREG_POWERSTATE_VALID, base + NvRegPowerState);

    nv_disable_hw_interrupts(dev, np->irqmask);
    pci_push(base);
    writel(NVREG_MIISTAT_MASK_ALL, base + NvRegMIIStatus);
    writel(NVREG_IRQSTAT_MASK, base + NvRegIrqStatus);
    pci_push(base);

    if (nv_request_irq(dev, 0)) {
        goto out_drain;
    }

    /* ask for interrupts */
    nv_enable_hw_interrupts(dev, np->irqmask);

    spinlock_disable(&np->lock);
    writel(NVREG_MCASTADDRA_FORCE, base + NvRegMulticastAddrA);
    writel(0, base + NvRegMulticastAddrB);
    writel(NVREG_MCASTMASKA_NONE, base + NvRegMulticastMaskA);
    writel(NVREG_MCASTMASKB_NONE, base + NvRegMulticastMaskB);
    writel(NVREG_PFF_ALWAYS|NVREG_PFF_MYADDR, base + NvRegPacketFilterFlags);
    /* One manual link speed update: Interrupts are enabled, future link
     * speed changes cause interrupts and are handled by nv_link_irq().
     */
    {
        uint32_t miistat;
        miistat = readl(base + NvRegMIIStatus);
        writel(NVREG_MIISTAT_MASK_ALL, base + NvRegMIIStatus);
        kprintf( INFO, "nvidia: startup: got 0x%08x.\n", miistat);
    }
    /* set linkspeed to invalid value, thus force nv_update_linkspeed
     * to init hw */
    np->linkspeed = 0;
    ret = nv_update_linkspeed(dev);
    nv_start_rxtx(dev);
    //netif_start_queue(dev);
    //nv_napi_enable(dev);

    if (ret) {
        net_device_carrier_on(dev);
    } else {
        kprintf( INFO, "nvidia: no link during initialization.\n" );
        net_device_carrier_off(dev);
    }

    if (oom)
        timer_setup(&np->oom_kick, get_system_time() + OOM_REFILL);

    /* start statistics timer */
    if (np->driver_data & (DEV_HAS_STATISTICS_V1|DEV_HAS_STATISTICS_V2|DEV_HAS_STATISTICS_V3))
        timer_setup(&np->stats_poll, get_system_time() + STATS_INTERVAL);

    spinunlock_enable(&np->lock);

    return 0;

out_drain:
    nv_drain_rxtx(dev);
    return ret;
}

static int nv_close(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base;

    spinlock_disable(&np->lock);
    np->in_shutdown = 1;
    spinunlock_enable(&np->lock);

    //nv_napi_disable(dev);
    //synchronize_irq(np->pci_dev->irq);

    timer_cancel(&np->oom_kick);
    timer_cancel(&np->nic_poll);
    timer_cancel(&np->stats_poll);

    //netif_stop_queue(dev);

    spinlock_disable(&np->lock);
    nv_stop_rxtx(dev);
    nv_txrx_reset(dev);

    /* disable interrupts on the nic or we will lock up */
    base = get_hwbase(dev);
    nv_disable_hw_interrupts(dev, np->irqmask);
    pci_push(base);

    spinunlock_enable(&np->lock);

    nv_free_irq(dev);
    nv_drain_rxtx(dev);

    if (np->wolenabled || !phy_power_down) {
        nv_txrx_gate(dev, false);
        writel(NVREG_PFF_ALWAYS|NVREG_PFF_MYADDR, base + NvRegPacketFilterFlags);
        nv_start_rx(dev);
    } else {
        /* power down phy */
        mii_rw(dev, np->phyaddr, MII_BMCR,
               mii_rw(dev, np->phyaddr, MII_BMCR, MII_READ)|BMCR_PDOWN);
        nv_txrx_gate(dev, true);
    }

    /* FIXME: power down nic */

    return 0;
}

static net_device_ops_t nv_netdev_ops = {
    .open = nv_open,
    .close = nv_close,
    .transmit = nv_start_xmit
};

static net_device_ops_t nv_netdev_ops_optimized = {
    .open = nv_open,
    .close = nv_close,
    .transmit = nv_start_xmit_optimized
};

static int nv_probe( pci_bus_t* pci_bus, pci_device_t* pci_device, nvidia_pci_entry_t* id )
{
    struct net_device *dev;
    struct fe_priv *np;
    ptr_t addr;
    uint8_t* base;
    int err, i;
    uint32_t powerstate, txreg;
    uint32_t phystate_orig = 0, phystate;
    int phyinitialized = 0;

    dev = net_device_create( sizeof( struct fe_priv ) );

    if ( dev == NULL ) {
        err = -ENOMEM;
        goto out;
    }

    np = net_device_get_private(dev);
    //np->dev = dev;
    //np->pci_dev = pci_dev;
    init_spinlock( &np->lock, "nVidia network lock" );
    //SET_NETDEV_DEV(dev, &pci_dev->dev);

    timer_init( &np->oom_kick, nv_do_rx_refill, dev );
    timer_init( &np->nic_poll, nv_do_nic_poll, dev );
    timer_init( &np->stats_poll, nv_do_stats_poll, dev );

    err = pci_bus->enable_device( pci_device, PCI_COMMAND_MASTER );
    if (err != 0)
        goto out_free;

    if (id->driver_data & (DEV_HAS_VLAN|DEV_HAS_MSI_X|DEV_HAS_POWER_CNTRL|DEV_HAS_STATISTICS_V2|DEV_HAS_STATISTICS_V3))
        np->register_size = NV_PCI_REGSZ_VER3;
    else if (id->driver_data & DEV_HAS_STATISTICS_V1)
        np->register_size = NV_PCI_REGSZ_VER2;
    else
        np->register_size = NV_PCI_REGSZ_VER1;

    for ( i = 0; i < 6; i++ ) {
        if ( ( ( pci_device->base[ i ] & PCI_ADDRESS_SPACE ) == PCI_ADDRESS_SPACE_MEMORY ) &&
             ( pci_device->size[ i ] >= np->register_size ) ) {
            addr = pci_device->base[ i ];
            break;
        }
    }

    if ( i == 6 ) {
        kprintf( INFO, "nvidia: Couldn't find register window.\n" );
        err = -EINVAL;
        goto out_relreg;
    }

    /* copy of driver data */
    np->driver_data = id->driver_data;

    /* handle different descriptor versions */
    if (id->driver_data & DEV_HAS_HIGH_DMA) {
        /* packet format 3: supports 40-bit addressing */
        np->desc_ver = DESC_VER_3;
        np->txrxctl_bits = NVREG_TXRXCTL_DESC_3;
    } else if (id->driver_data & DEV_HAS_LARGEDESC) {
        /* packet format 2: supports jumbo frames */
        np->desc_ver = DESC_VER_2;
        np->txrxctl_bits = NVREG_TXRXCTL_DESC_2;
    } else {
        /* original packet format */
        np->desc_ver = DESC_VER_1;
        np->txrxctl_bits = NVREG_TXRXCTL_DESC_1;
    }

    np->pkt_limit = NV_PKTLIMIT_1;
    if (id->driver_data & DEV_HAS_LARGEDESC)
        np->pkt_limit = NV_PKTLIMIT_2;

    if (id->driver_data & DEV_HAS_CHECKSUM) {
        np->rx_csum = 1;
        np->txrxctl_bits |= NVREG_TXRXCTL_RXCHECK;
        //dev->features |= NETIF_F_IP_CSUM | NETIF_F_SG;
        //dev->features |= NETIF_F_TSO;
    }

    np->vlanctl_bits = 0;
    if (id->driver_data & DEV_HAS_VLAN) {
        np->vlanctl_bits = NVREG_VLANCONTROL_ENABLE;
        //dev->features |= NETIF_F_HW_VLAN_RX | NETIF_F_HW_VLAN_TX;
    }

    np->pause_flags = NV_PAUSEFRAME_RX_CAPABLE | NV_PAUSEFRAME_RX_REQ | NV_PAUSEFRAME_AUTONEG;
    if ((id->driver_data & DEV_HAS_PAUSEFRAME_TX_V1) ||
        (id->driver_data & DEV_HAS_PAUSEFRAME_TX_V2) ||
        (id->driver_data & DEV_HAS_PAUSEFRAME_TX_V3)) {
        np->pause_flags |= NV_PAUSEFRAME_TX_CAPABLE | NV_PAUSEFRAME_TX_REQ;
    }

    np->region = memory_region_create( "nVidia registers", PAGE_ALIGN( np->register_size ),
                                       REGION_KERNEL | REGION_READ | REGION_WRITE );
    /* todo: error checking */
    memory_region_remap_pages( np->region, addr );
    np->base = ( void* )np->region->address;

    np->dev_irq = pci_device->interrupt_line;

    np->rx_ring_size = RX_RING_DEFAULT;
    np->tx_ring_size = TX_RING_DEFAULT;

    if (!nv_optimized(np)) {
        np->rx_ring.orig = kmalloc(sizeof(struct ring_desc) * (np->rx_ring_size + np->tx_ring_size));
        if (!np->rx_ring.orig)
            goto out_unmap;
        np->ring_addr = (uint32_t)np->rx_ring.orig;
        np->tx_ring.orig = &np->rx_ring.orig[np->rx_ring_size];
    } else {
        np->rx_ring.ex = kmalloc(sizeof(struct ring_desc_ex) * (np->rx_ring_size + np->tx_ring_size));
        if (!np->rx_ring.ex)
            goto out_unmap;
        np->ring_addr = (uint32_t)np->rx_ring.ex;
        np->tx_ring.ex = &np->rx_ring.ex[np->rx_ring_size];
    }
    np->rx_skb = kcalloc(np->rx_ring_size, sizeof(struct nv_skb_map));
    np->tx_skb = kcalloc(np->tx_ring_size, sizeof(struct nv_skb_map));
    if (!np->rx_skb || !np->tx_skb)
        goto out_freering;

    if (!nv_optimized(np)) {
        dev->ops = &nv_netdev_ops;
    } else {
        dev->ops = &nv_netdev_ops_optimized;
    }

    //SET_ETHTOOL_OPS(dev, &ops);
    //dev->watchdog_timeo = NV_WATCHDOG_TIMEO;

    //pci_set_drvdata(pci_dev, dev);

    /* read the mac address */
    base = get_hwbase(dev);
    np->orig_mac[0] = readl(base + NvRegMacAddrA);
    np->orig_mac[1] = readl(base + NvRegMacAddrB);

    /* check the workaround bit for correct mac address order */
    txreg = readl(base + NvRegTransmitPoll);
    if (id->driver_data & DEV_HAS_CORRECT_MACADDR) {
        /* mac address is already in correct order */
        dev->dev_addr[0] = (np->orig_mac[0] >>  0) & 0xff;
        dev->dev_addr[1] = (np->orig_mac[0] >>  8) & 0xff;
        dev->dev_addr[2] = (np->orig_mac[0] >> 16) & 0xff;
        dev->dev_addr[3] = (np->orig_mac[0] >> 24) & 0xff;
        dev->dev_addr[4] = (np->orig_mac[1] >>  0) & 0xff;
        dev->dev_addr[5] = (np->orig_mac[1] >>  8) & 0xff;
    } else if (txreg & NVREG_TRANSMITPOLL_MAC_ADDR_REV) {
        /* mac address is already in correct order */
        dev->dev_addr[0] = (np->orig_mac[0] >>  0) & 0xff;
        dev->dev_addr[1] = (np->orig_mac[0] >>  8) & 0xff;
        dev->dev_addr[2] = (np->orig_mac[0] >> 16) & 0xff;
        dev->dev_addr[3] = (np->orig_mac[0] >> 24) & 0xff;
        dev->dev_addr[4] = (np->orig_mac[1] >>  0) & 0xff;
        dev->dev_addr[5] = (np->orig_mac[1] >>  8) & 0xff;
        /*
         * Set orig mac address back to the reversed version.
         * This flag will be cleared during low power transition.
         * Therefore, we should always put back the reversed address.
         */
        np->orig_mac[0] = (dev->dev_addr[5] << 0) + (dev->dev_addr[4] << 8) +
            (dev->dev_addr[3] << 16) + (dev->dev_addr[2] << 24);
        np->orig_mac[1] = (dev->dev_addr[1] << 0) + (dev->dev_addr[0] << 8);
    } else {
        /* need to reverse mac address to correct order */
        dev->dev_addr[0] = (np->orig_mac[1] >>  8) & 0xff;
        dev->dev_addr[1] = (np->orig_mac[1] >>  0) & 0xff;
        dev->dev_addr[2] = (np->orig_mac[0] >> 24) & 0xff;
        dev->dev_addr[3] = (np->orig_mac[0] >> 16) & 0xff;
        dev->dev_addr[4] = (np->orig_mac[0] >>  8) & 0xff;
        dev->dev_addr[5] = (np->orig_mac[0] >>  0) & 0xff;
        writel(txreg|NVREG_TRANSMITPOLL_MAC_ADDR_REV, base + NvRegTransmitPoll);
        DEBUG_LOG( "nv_probe: set workaround bit for reversed mac addr\n");
    }

    if (!is_valid_ethernet_address(dev->dev_addr)) {
        /*
         * Bad mac address. At least one bios sets the mac address
         * to 01:23:45:67:89:ab
         */
        kprintf(ERROR, "nvidia: Invalid Mac address detected: %x\n",
                dev->dev_addr);
        kprintf(ERROR, "nvidia: Please complain to your hardware vendor. Switching to a random MAC.\n");
        random_ethernet_address(dev->dev_addr);
    }

    /* set mac address */
    nv_copy_mac_to_hw(dev);

    /* Workaround current PCI init glitch:  wakeup bits aren't
     * being set from PCI PM capability.
     */
    //device_init_wakeup(&pci_dev->dev, 1);

    /* disable WOL */
    writel(0, base + NvRegWakeUpFlags);
    np->wolenabled = 0;

    if (id->driver_data & DEV_HAS_POWER_CNTRL) {

        /* take phy and nic out of low power mode */
        powerstate = readl(base + NvRegPowerState2);
        powerstate &= ~NVREG_POWERSTATE2_POWERUP_MASK;
        if ((id->driver_data & DEV_NEED_LOW_POWER_FIX) &&
            pci_device->revision_id >= 0xA3)
            powerstate |= NVREG_POWERSTATE2_POWERUP_REV_A3;
        writel(powerstate, base + NvRegPowerState2);
    }

    if (np->desc_ver == DESC_VER_1) {
        np->tx_flags = NV_TX_VALID;
    } else {
        np->tx_flags = NV_TX2_VALID;
    }

    if (optimization_mode == NV_OPTIMIZATION_MODE_CPU) {
        np->irqmask = NVREG_IRQMASK_CPU;
    } else if (optimization_mode == NV_OPTIMIZATION_MODE_DYNAMIC &&
           !(id->driver_data & DEV_NEED_TIMERIRQ)) {
        /* start off in throughput mode */
        np->irqmask = NVREG_IRQMASK_THROUGHPUT;
    } else {
        optimization_mode = NV_OPTIMIZATION_MODE_THROUGHPUT;
        np->irqmask = NVREG_IRQMASK_THROUGHPUT;
    }

    if (id->driver_data & DEV_NEED_TIMERIRQ)
        np->irqmask |= NVREG_IRQ_TIMER;
    if (id->driver_data & DEV_NEED_LINKTIMER) {
        kprintf( INFO, "nvidia: link timer on.\n" );
        np->need_linktimer = 1;
        np->link_timeout = get_system_time() + LINK_TIMEOUT;
    } else {
        kprintf( INFO, "nvidia: link timer off.\n" );
        np->need_linktimer = 0;
    }

    /* Limit the number of tx's outstanding for hw bug */
    if (id->driver_data & DEV_NEED_TX_LIMIT) {
        np->tx_limit = 1;
        if ((id->driver_data & DEV_NEED_TX_LIMIT2) &&
            pci_device->revision_id >= 0xA2)
            np->tx_limit = 0;
    }

    /* clear phy state and temporarily halt phy interrupts */
    writel(0, base + NvRegMIIMask);
    phystate = readl(base + NvRegAdapterControl);
    if (phystate & NVREG_ADAPTCTL_RUNNING) {
        phystate_orig = 1;
        phystate &= ~NVREG_ADAPTCTL_RUNNING;
        writel(phystate, base + NvRegAdapterControl);
    }
    writel(NVREG_MIISTAT_MASK_ALL, base + NvRegMIIStatus);

    if (id->driver_data & DEV_HAS_MGMT_UNIT) {
        /* management unit running on the mac? */
        if ((readl(base + NvRegTransmitterControl) & NVREG_XMITCTL_MGMT_ST) &&
            (readl(base + NvRegTransmitterControl) & NVREG_XMITCTL_SYNC_PHY_INIT) &&
            nv_mgmt_acquire_sema(dev) &&
            nv_mgmt_get_version(dev)) {
            np->mac_in_use = 1;
            if (np->mgmt_version > 0) {
                np->mac_in_use = readl(base + NvRegMgmtUnitControl) & NVREG_MGMTUNITCONTROL_INUSE;
            }
            kprintf( INFO, "nvidia: mgmt unit is running. mac in use %x.\n",np->mac_in_use);
            /* management unit setup the phy already? */
            if (np->mac_in_use &&
                ((readl(base + NvRegTransmitterControl) & NVREG_XMITCTL_SYNC_MASK) ==
                 NVREG_XMITCTL_SYNC_PHY_INIT)) {
                /* phy is inited by mgmt unit */
                phyinitialized = 1;
                kprintf( INFO, "nvidia: Phy already initialized by mgmt unit.\n" );
            } else {
                /* we need to init the phy */
            }
        }
    }

    /* find a suitable phy */
    for (i = 1; i <= 32; i++) {
        int id1, id2;
        int phyaddr = i & 0x1F;

        spinlock_disable(&np->lock);
        id1 = mii_rw(dev, phyaddr, MII_PHYSID1, MII_READ);
        spinunlock_enable(&np->lock);
        if (id1 < 0 || id1 == 0xffff)
            continue;
        spinlock_disable(&np->lock);
        id2 = mii_rw(dev, phyaddr, MII_PHYSID2, MII_READ);
        spinunlock_enable(&np->lock);
        if (id2 < 0 || id2 == 0xffff)
            continue;

        np->phy_model = id2 & PHYID2_MODEL_MASK;
        id1 = (id1 & PHYID1_OUI_MASK) << PHYID1_OUI_SHFT;
        id2 = (id2 & PHYID2_OUI_MASK) >> PHYID2_OUI_SHFT;
        DEBUG_LOG( "nvidia: open: Found PHY %04x:%04x at address %d.\n", id1, id2, phyaddr);
        np->phyaddr = phyaddr;
        np->phy_oui = id1 | id2;

        /* Realtek hardcoded phy id1 to all zero's on certain phys */
        if (np->phy_oui == PHY_OUI_REALTEK2)
            np->phy_oui = PHY_OUI_REALTEK;
        /* Setup phy revision for Realtek */
        if (np->phy_oui == PHY_OUI_REALTEK && np->phy_model == PHY_MODEL_REALTEK_8211)
            np->phy_rev = mii_rw(dev, phyaddr, MII_RESV1, MII_READ) & PHY_REV_MASK;

        break;
    }
    if (i == 33) {
        kprintf(INFO, "nvidia: open: Could not find a valid PHY.\n" );
        goto out_error;
    }

    if (!phyinitialized) {
        /* reset it */
        phy_init(dev);
    } else {
        /* see if it is a gigabit phy */
        uint32_t mii_status = mii_rw(dev, np->phyaddr, MII_BMSR, MII_READ);
        if (mii_status & PHY_GIGABIT) {
            np->gigabit = PHY_GIGABIT;
        }
    }

    /* set default link speed settings */
    np->linkspeed = NVREG_LINKSPEED_FORCE|NVREG_LINKSPEED_10;
    np->duplex = 0;
    np->autoneg = 1;

    err = net_device_register(dev);
    if (err != 0) {
        kprintf(INFO, "nvidia: unable to register netdev: %d\n", err);
        goto out_error;
    }

#if 0
    kprintf(INFO, "ifname %s, PHY OUI 0x%x @ %d, "
           "addr %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n",
           dev->name,
           np->phy_oui,
           np->phyaddr,
           dev->dev_addr[0],
           dev->dev_addr[1],
           dev->dev_addr[2],
           dev->dev_addr[3],
           dev->dev_addr[4],
           dev->dev_addr[5]);

    kprintf(INFO, "%s%s%s%s%s%s%s%sdesc-v%u\n",
           dev->features & NETIF_F_HIGHDMA ? "highdma " : "",
           dev->features & (NETIF_F_IP_CSUM | NETIF_F_SG) ?
            "csum " : "",
           dev->features & (NETIF_F_HW_VLAN_RX | NETIF_F_HW_VLAN_TX) ?
            "vlan " : "",
           id->driver_data & DEV_HAS_POWER_CNTRL ? "pwrctl " : "",
           id->driver_data & DEV_HAS_MGMT_UNIT ? "mgmt " : "",
           id->driver_data & DEV_NEED_TIMERIRQ ? "timirq " : "",
           np->gigabit == PHY_GIGABIT ? "gbit " : "",
           np->need_linktimer ? "lnktim " : "",
           np->desc_ver);
#endif

    return 0;

out_error:
    if (phystate_orig)
        writel(phystate|NVREG_ADAPTCTL_RUNNING, base + NvRegAdapterControl);

out_freering:
    free_rings(dev);

out_unmap:
    memory_region_put(np->region);
    np->region = NULL;

out_relreg:
    //pci_release_regions(pci_dev);

out_free:
    net_device_free(dev);

out:
    return err;
}

#if 0
static void nv_restore_phy(struct net_device *dev)
{
    struct fe_priv *np = net_device_get_private(dev);
    uint16_t phy_reserved, mii_control;

    if (np->phy_oui == PHY_OUI_REALTEK &&
        np->phy_model == PHY_MODEL_REALTEK_8201 &&
        phy_cross == NV_CROSSOVER_DETECTION_DISABLED) {
        mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT3);
        phy_reserved = mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG2, MII_READ);
        phy_reserved &= ~PHY_REALTEK_INIT_MSK1;
        phy_reserved |= PHY_REALTEK_INIT8;
        mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG2, phy_reserved);
        mii_rw(dev, np->phyaddr, PHY_REALTEK_INIT_REG1, PHY_REALTEK_INIT1);

        /* restart auto negotiation */
        mii_control = mii_rw(dev, np->phyaddr, MII_BMCR, MII_READ);
        mii_control |= (BMCR_ANRESTART | BMCR_ANENABLE);
        mii_rw(dev, np->phyaddr, MII_BMCR, mii_control);
    }
}

static void nv_restore_mac_addr(struct pci_dev *pci_dev)
{
    struct net_device *dev = pci_get_drvdata(pci_dev);
    struct fe_priv *np = net_device_get_private(dev);
    uint8_t* base = get_hwbase(dev);

    /* special op: write back the misordered MAC address - otherwise
     * the next nv_probe would see a wrong address.
     */
    writel(np->orig_mac[0], base + NvRegMacAddrA);
    writel(np->orig_mac[1], base + NvRegMacAddrB);
    writel(readl(base + NvRegTransmitPoll) & ~NVREG_TRANSMITPOLL_MAC_ADDR_REV,
           base + NvRegTransmitPoll);
}

static void nv_remove(struct pci_dev *pci_dev)
{
    struct net_device *dev = pci_get_drvdata(pci_dev);

    unregister_netdev(dev);

    nv_restore_mac_addr(pci_dev);

    /* restore any phy related changes */
    nv_restore_phy(dev);

    nv_mgmt_release_sema(dev);

    /* free all structures */
    free_rings(dev);
    iounmap(get_hwbase(dev));
    pci_release_regions(pci_dev);
    pci_disable_device(pci_dev);
    free_netdev(dev);
    pci_set_drvdata(pci_dev, NULL);
}
#endif

static nvidia_pci_entry_t pci_table[] = {
    {   /* nForce Ethernet Controller */
        0x10DE, 0x01C3,
        DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER,
    },
    {   /* nForce2 Ethernet Controller */
        0x10DE, 0x0066,
        DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER,
    },
    {   /* nForce3 Ethernet Controller */
        0x10DE, 0x00D6,
        DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER,
    },
    {   /* nForce3 Ethernet Controller */
        0x10DE, 0x0086,
        DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM,
    },
    {   /* nForce3 Ethernet Controller */
        0x10DE, 0x008C,
        DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM,
    },
    {   /* nForce3 Ethernet Controller */
        0x10DE, 0x00E6,
        DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM,
    },
    {   /* nForce3 Ethernet Controller */
        0x10DE, 0x00DF,
        DEV_NEED_TIMERIRQ|DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM,
    },
    {   /* CK804 Ethernet Controller */
        0x10DE, 0x0056,
        DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_STATISTICS_V1|DEV_NEED_TX_LIMIT,
    },
    {   /* CK804 Ethernet Controller */
        0x10DE, 0x0057,
        DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_STATISTICS_V1|DEV_NEED_TX_LIMIT,
    },
    {   /* MCP04 Ethernet Controller */
        0x10DE, 0x0037,
        DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_STATISTICS_V1|DEV_NEED_TX_LIMIT,
    },
    {   /* MCP04 Ethernet Controller */
        0x10DE, 0x0038,
        DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_STATISTICS_V1|DEV_NEED_TX_LIMIT,
    },
    {   /* MCP51 Ethernet Controller */
        0x10DE, 0x0268,
        DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_STATISTICS_V1|DEV_NEED_LOW_POWER_FIX,
    },
    {   /* MCP51 Ethernet Controller */
        0x10DE, 0x0269,
        DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_STATISTICS_V1|DEV_NEED_LOW_POWER_FIX,
    },
    {   /* MCP55 Ethernet Controller */
        0x10DE, 0x0372,
        DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_VLAN|DEV_HAS_MSI|DEV_HAS_MSI_X|DEV_HAS_POWER_CNTRL|DEV_HAS_PAUSEFRAME_TX_V1|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_NEED_TX_LIMIT|DEV_NEED_MSI_FIX,
    },
    {   /* MCP55 Ethernet Controller */
        0x10DE, 0x0373,
        DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_VLAN|DEV_HAS_MSI|DEV_HAS_MSI_X|DEV_HAS_POWER_CNTRL|DEV_HAS_PAUSEFRAME_TX_V1|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_NEED_TX_LIMIT|DEV_NEED_MSI_FIX,
    },
    {   /* MCP61 Ethernet Controller */
        0x10DE, 0x03E5,
        DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX_V1|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_NEED_MSI_FIX,
    },
    {   /* MCP61 Ethernet Controller */
        0x10DE, 0x03E6,
        DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX_V1|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_NEED_MSI_FIX,
    },
    {   /* MCP61 Ethernet Controller */
        0x10DE, 0x03EE,
        DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX_V1|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_NEED_MSI_FIX,
    },
    {   /* MCP61 Ethernet Controller */
        0x10DE, 0x03EF,
        DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX_V1|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_NEED_MSI_FIX,
    },
    {   /* MCP65 Ethernet Controller */
        0x10DE, 0x0450,
        DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX_V1|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_NEED_TX_LIMIT|DEV_HAS_GEAR_MODE|DEV_NEED_MSI_FIX,
    },
    {   /* MCP65 Ethernet Controller */
        0x10DE, 0x0451,
        DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX_V1|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_NEED_TX_LIMIT|DEV_HAS_GEAR_MODE|DEV_NEED_MSI_FIX,
    },
    {   /* MCP65 Ethernet Controller */
        0x10DE, 0x0452,
        DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX_V1|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_NEED_TX_LIMIT|DEV_HAS_GEAR_MODE|DEV_NEED_MSI_FIX,
    },
    {   /* MCP65 Ethernet Controller */
        0x10DE, 0x0453,
        DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX_V1|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_NEED_TX_LIMIT|DEV_HAS_GEAR_MODE|DEV_NEED_MSI_FIX,
    },
    {   /* MCP67 Ethernet Controller */
        0x10DE, 0x054C,
        DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX_V1|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_HAS_GEAR_MODE|DEV_NEED_MSI_FIX,
    },
    {   /* MCP67 Ethernet Controller */
        0x10DE, 0x054D,
        DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX_V1|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_HAS_GEAR_MODE|DEV_NEED_MSI_FIX,
    },
    {   /* MCP67 Ethernet Controller */
        0x10DE, 0x054E,
        DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX_V1|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_HAS_GEAR_MODE|DEV_NEED_MSI_FIX,
    },
    {   /* MCP67 Ethernet Controller */
        0x10DE, 0x054F,
        DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX_V1|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_HAS_GEAR_MODE|DEV_NEED_MSI_FIX,
    },
    {   /* MCP73 Ethernet Controller */
        0x10DE, 0x07DC,
        DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX_V1|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX|DEV_HAS_GEAR_MODE|DEV_NEED_MSI_FIX,
    },
    {   /* MCP73 Ethernet Controller */
        0x10DE, 0x07DD,
        DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX_V1|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX|DEV_HAS_GEAR_MODE|DEV_NEED_MSI_FIX,
    },
    {   /* MCP73 Ethernet Controller */
        0x10DE, 0x07DE,
        DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX_V1|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX|DEV_HAS_GEAR_MODE|DEV_NEED_MSI_FIX,
    },
    {   /* MCP73 Ethernet Controller */
        0x10DE, 0x07DF,
        DEV_NEED_LINKTIMER|DEV_HAS_HIGH_DMA|DEV_HAS_POWER_CNTRL|DEV_HAS_MSI|DEV_HAS_PAUSEFRAME_TX_V1|DEV_HAS_STATISTICS_V2|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX|DEV_HAS_GEAR_MODE|DEV_NEED_MSI_FIX,
    },
    {   /* MCP77 Ethernet Controller */
        0x10DE, 0x0760,
        DEV_NEED_LINKTIMER|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_MSI|DEV_HAS_POWER_CNTRL|DEV_HAS_PAUSEFRAME_TX_V2|DEV_HAS_STATISTICS_V3|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX|DEV_NEED_TX_LIMIT2|DEV_HAS_GEAR_MODE|DEV_NEED_PHY_INIT_FIX|DEV_NEED_MSI_FIX,
    },
    {   /* MCP77 Ethernet Controller */
        0x10DE, 0x0761,
        DEV_NEED_LINKTIMER|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_MSI|DEV_HAS_POWER_CNTRL|DEV_HAS_PAUSEFRAME_TX_V2|DEV_HAS_STATISTICS_V3|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX|DEV_NEED_TX_LIMIT2|DEV_HAS_GEAR_MODE|DEV_NEED_PHY_INIT_FIX|DEV_NEED_MSI_FIX,
    },
    {   /* MCP77 Ethernet Controller */
        0x10DE, 0x0762,
        DEV_NEED_LINKTIMER|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_MSI|DEV_HAS_POWER_CNTRL|DEV_HAS_PAUSEFRAME_TX_V2|DEV_HAS_STATISTICS_V3|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX|DEV_NEED_TX_LIMIT2|DEV_HAS_GEAR_MODE|DEV_NEED_PHY_INIT_FIX|DEV_NEED_MSI_FIX,
    },
    {   /* MCP77 Ethernet Controller */
        0x10DE, 0x0763,
        DEV_NEED_LINKTIMER|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_MSI|DEV_HAS_POWER_CNTRL|DEV_HAS_PAUSEFRAME_TX_V2|DEV_HAS_STATISTICS_V3|DEV_HAS_TEST_EXTENDED|DEV_HAS_MGMT_UNIT|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX|DEV_NEED_TX_LIMIT2|DEV_HAS_GEAR_MODE|DEV_NEED_PHY_INIT_FIX|DEV_NEED_MSI_FIX,
    },
    {   /* MCP79 Ethernet Controller */
        0x10DE, 0x0AB0,
        DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_MSI|DEV_HAS_POWER_CNTRL|DEV_HAS_PAUSEFRAME_TX_V3|DEV_HAS_STATISTICS_V3|DEV_HAS_TEST_EXTENDED|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX|DEV_NEED_TX_LIMIT2|DEV_HAS_GEAR_MODE|DEV_NEED_PHY_INIT_FIX|DEV_NEED_MSI_FIX,
    },
    {   /* MCP79 Ethernet Controller */
        0x10DE, 0x0AB1,
        DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_MSI|DEV_HAS_POWER_CNTRL|DEV_HAS_PAUSEFRAME_TX_V3|DEV_HAS_STATISTICS_V3|DEV_HAS_TEST_EXTENDED|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX|DEV_NEED_TX_LIMIT2|DEV_HAS_GEAR_MODE|DEV_NEED_PHY_INIT_FIX|DEV_NEED_MSI_FIX,
    },
    {   /* MCP79 Ethernet Controller */
        0x10DE, 0x0AB2,
        DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_MSI|DEV_HAS_POWER_CNTRL|DEV_HAS_PAUSEFRAME_TX_V3|DEV_HAS_STATISTICS_V3|DEV_HAS_TEST_EXTENDED|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX|DEV_NEED_TX_LIMIT2|DEV_HAS_GEAR_MODE|DEV_NEED_PHY_INIT_FIX|DEV_NEED_MSI_FIX,
    },
    {   /* MCP79 Ethernet Controller */
        0x10DE, 0x0AB3,
        DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_MSI|DEV_HAS_POWER_CNTRL|DEV_HAS_PAUSEFRAME_TX_V3|DEV_HAS_STATISTICS_V3|DEV_HAS_TEST_EXTENDED|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX|DEV_NEED_TX_LIMIT2|DEV_HAS_GEAR_MODE|DEV_NEED_PHY_INIT_FIX|DEV_NEED_MSI_FIX,
    },
    {   /* MCP89 Ethernet Controller */
        0x10DE, 0x0D7D,
        DEV_NEED_LINKTIMER|DEV_HAS_LARGEDESC|DEV_HAS_CHECKSUM|DEV_HAS_HIGH_DMA|DEV_HAS_MSI|DEV_HAS_POWER_CNTRL|DEV_HAS_PAUSEFRAME_TX_V3|DEV_HAS_STATISTICS_V3|DEV_HAS_TEST_EXTENDED|DEV_HAS_CORRECT_MACADDR|DEV_HAS_COLLISION_FIX|DEV_HAS_GEAR_MODE|DEV_NEED_PHY_INIT_FIX,
    },
};

int init_module( void ) {
    int i;
    int dev_count;
    pci_bus_t* pci_bus;

    pci_bus = get_bus_driver( "PCI" );

    if ( pci_bus == NULL ) {
        kprintf( INFO, "nvidia: PCI bus not found.\n" );
        return -1;
    }

    dev_count = pci_bus->get_device_count();

    for ( i = 0; i < dev_count; i++ ) {
        int j;
        pci_device_t* pci_device;

        pci_device = pci_bus->get_device( i );

        for ( j = 0; j < ARRAY_SIZE( pci_table ); j++ ) {
            nvidia_pci_entry_t* entry;

            entry = &pci_table[ j ];

            if ( ( pci_device->vendor_id == entry->vendor_id ) &&
                 ( pci_device->device_id == entry->device_id ) ) {
                nv_probe( pci_bus, pci_device, entry );
                break;
            }
        }
    }

    return 0;
}

int destroy_module( void ) {
    return 0;
}
