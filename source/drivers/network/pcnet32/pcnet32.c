/* PCnet32 driver (based on the Linux driver)
 *
 * Copyright (c) 2009 Zoltan Kovacs
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
#include <macros.h>
#include <devices.h>
#include <errno.h>
#include <ioctl.h>
#include <irq.h>
#include <mm/kmalloc.h>
#include <vfs/devfs.h>
#include <lib/string.h>

#include <arch/io.h>
#include <arch/smp.h>

#include "../../bus/pci/pci.h"

#include "pcnet32.h"

#define PKT_BUF_SKB 1544

static int cards_found = 0;
static int max_interrupt_work = 40;

/* TODO: move to network stuff */
static inline int is_valid_ether_addr( uint8_t* address ) {
    return 1;
}

static pcnet32_pci_entry_t pci_table[] = {
    { PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_AMD_LANCE },
    { PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_AMD_LANCE_HOME }
};

static uint16_t pcnet32_wio_read_csr( uint32_t addr, int index ) {
    outw( index, addr + PCNET32_WIO_RAP );
    return inw( addr + PCNET32_WIO_RDP );
}

static void pcnet32_wio_write_csr( uint32_t addr, int index, uint16_t value ) {
    outw( index, addr + PCNET32_WIO_RAP );
    outw( value, addr + PCNET32_WIO_RDP );
}

static uint16_t pcnet32_wio_read_bcr( uint32_t addr, int index ) {
    outw( index, addr + PCNET32_WIO_RAP );
    return inw( addr + PCNET32_WIO_BDP );
}

static void pcnet32_wio_write_bcr( uint32_t addr, int index, uint16_t value ) {
    outw( index, addr + PCNET32_WIO_RAP );
    outw( value, addr + PCNET32_WIO_BDP );
}

static uint16_t pcnet32_wio_read_rap( uint32_t addr ) {
    return inw( addr + PCNET32_WIO_RAP );
}

static void pcnet32_wio_write_rap( uint32_t addr, uint16_t value ) {
    outw( value, addr + PCNET32_WIO_RAP );
}

static void pcnet32_wio_reset( uint32_t addr ) {
    inw( addr + PCNET32_WIO_RESET );
}

static int pcnet32_wio_check( uint32_t addr ) {
    outw( 88, addr + PCNET32_WIO_RAP );
    return ( inw( addr + PCNET32_WIO_RAP ) == 88 );
}

static pcnet32_access_t pcnet32_wio = {
    .read_csr = pcnet32_wio_read_csr,
    .write_csr = pcnet32_wio_write_csr,
    .read_bcr = pcnet32_wio_read_bcr,
    .write_bcr = pcnet32_wio_write_bcr,
    .read_rap = pcnet32_wio_read_rap,
    .write_rap = pcnet32_wio_write_rap,
    .reset = pcnet32_wio_reset
};

static uint16_t pcnet32_dwio_read_csr( uint32_t addr, int index ) {
    outl( index, addr + PCNET32_DWIO_RAP );
    return ( inl( addr + PCNET32_DWIO_RDP ) & 0xFFFF );
}

static void pcnet32_dwio_write_csr( uint32_t addr, int index, uint16_t value ) {
    outl( index, addr + PCNET32_DWIO_RAP );
    outl( value, addr + PCNET32_DWIO_RDP );
}

static uint16_t pcnet32_dwio_read_bcr( uint32_t addr, int index ) {
    outl( index, addr + PCNET32_DWIO_RAP );
    return ( inl( addr + PCNET32_DWIO_BDP ) & 0xFFFF );
}

static void pcnet32_dwio_write_bcr( uint32_t addr, int index, uint16_t value ) {
    outl( index, addr + PCNET32_DWIO_RAP );
    outl( value, addr + PCNET32_DWIO_BDP );
}

static uint16_t pcnet32_dwio_read_rap( uint32_t addr ) {
    return ( inl( addr + PCNET32_DWIO_RAP ) & 0xFFFF );
}

static void pcnet32_dwio_write_rap( uint32_t addr, uint16_t value ) {
    outl( value, addr + PCNET32_DWIO_RAP );
}

static void pcnet32_dwio_reset( uint32_t addr ) {
    inl( addr + PCNET32_DWIO_RESET );
}

static int pcnet32_dwio_check( uint32_t addr ) {
    outl( 88, addr + PCNET32_DWIO_RAP );
    return ( ( inl( addr + PCNET32_DWIO_RAP ) & 0xFFFF ) == 88 );
}

static pcnet32_access_t pcnet32_dwio = {
    .read_csr = pcnet32_dwio_read_csr,
    .write_csr = pcnet32_dwio_write_csr,
    .read_bcr = pcnet32_dwio_read_bcr,
    .write_bcr = pcnet32_dwio_write_bcr,
    .read_rap = pcnet32_dwio_read_rap,
    .write_rap = pcnet32_dwio_write_rap,
    .reset = pcnet32_dwio_reset
};

static int mdio_read( pcnet32_private_t* device, int phy_id, int reg_num ) {
    int io_address;

    if ( !device->mii ) {
        return 0;
    }

    io_address = device->io_address;

    device->access->write_bcr( io_address, 33, ( ( phy_id & 0x1F ) << 5 ) | ( reg_num & 0x1F ) );
    return device->access->read_bcr( io_address, 34 );
}

static void mdio_write( pcnet32_private_t* device, int phy_id, int reg_num, int value ) {
    int io_address;

    if ( !device->mii ) {
        return;
    }

    io_address = device->io_address;

    device->access->write_bcr( io_address, 33, ( ( phy_id & 0x1F ) << 5 ) | ( reg_num & 0x1F ) );
    device->access->write_bcr( io_address, 34, value );
}

static int pcnet32_alloc_ring( pcnet32_private_t* device ) {
    uint32_t tmp;

    device->tx_ring_base = kmalloc( sizeof( pcnet32_tx_head_t ) * device->tx_ring_size + 15 );

    if ( device->tx_ring_base == NULL ) {
        return -ENOMEM;
    }

    device->rx_ring_base = kmalloc( sizeof( pcnet32_rx_head_t ) * device->rx_ring_size + 15 );

    if ( device->rx_ring_base == NULL ) {
        return -ENOMEM;
    }

    device->tx_packet_buf = ( packet_t** )kmalloc( sizeof( packet_t* ) * device->tx_ring_size );

    if ( device->tx_packet_buf == NULL ) {
        return -ENOMEM;
    }

    device->rx_packet_buf = ( packet_t** )kmalloc( sizeof( packet_t* ) * device->rx_ring_size );

    if ( device->rx_packet_buf == NULL ) {
        return -ENOMEM;
    }

    tmp = ( uint32_t )device->tx_ring_base;
    tmp = ( tmp + 15 ) & ~15;
    device->tx_ring = ( pcnet32_tx_head_t* )tmp;

    tmp = ( uint32_t )device->rx_ring_base;
    tmp = ( tmp + 15 ) & ~15;
    device->rx_ring = ( pcnet32_rx_head_t* )tmp;

    memset( device->tx_ring, 0, sizeof( pcnet32_tx_head_t ) * device->tx_ring_size );
    memset( device->rx_ring, 0, sizeof( pcnet32_rx_head_t ) * device->rx_ring_size );
    memset( device->tx_packet_buf, 0, sizeof( packet_t* ) * device->tx_ring_size );
    memset( device->rx_packet_buf, 0, sizeof( packet_t* ) * device->rx_ring_size );

    return 0;
}

static void pcnet32_rx_entry( pcnet32_private_t* device, pcnet32_rx_head_t* rxp, int entry ) {
    int status;
    int rx_in_place;
    short pkt_len;
    packet_t* packet;
    packet_t* new_packet;

    status = rxp->status >> 8;
    rx_in_place = 0;

    if ( status != 0x03 ) {   /* There was an error. */
        if ( status & 0x01 ) {
            device->stats.rx_errors++;
        }

        return;
    }

    pkt_len = ( rxp->msg_length & 0xFFF ) - 4;

    /* Discard oversize frames. */

    if ( pkt_len > PKT_BUF_SKB ) {
        kprintf( ERROR, "PCnet32: Impossible packet size %d!\n", pkt_len );
        device->stats.rx_errors++;
        return;
    }

    if ( pkt_len < 60 ) {
        kprintf( ERROR, "PCnet32: Runt packet!\n" );
        device->stats.rx_errors++;
        return;
    }

    new_packet = create_packet( PKT_BUF_SKB );

    if ( new_packet == NULL ) {
        kprintf( ERROR, "PCnet32: No memory for new packet buffer!\n" );
        return;
    }

    packet = device->rx_packet_buf[ entry ];

    device->rx_packet_buf[ entry ] = new_packet;
    rxp->base = ( uint32_t )new_packet->data;

    /* Make sure owner changes after all others are visible */

    wmb();

    packet->size = pkt_len;
    ASSERT( device->input_queue != NULL );
    packet_queue_insert( device->input_queue, packet );

    device->stats.rx_bytes += pkt_len;
    device->stats.rx_packets++;
}

static int pcnet32_rx( pcnet32_private_t* device ) {
    int entry;
    int npackets;
    pcnet32_rx_head_t* rxp;

    npackets = 0;
    entry = device->cur_rx & device->rx_mod_mask;
    rxp = &device->rx_ring[ entry ];

    while ( ( short )rxp->status >= 0 ) {
        pcnet32_rx_entry( device, rxp, entry );
        npackets += 1;

        rxp->buf_length = -PKT_BUF_SKB;

        /* Make sure owner changes after others are visible */

        wmb();

        rxp->status = 0x8000;

        entry = ( ++device->cur_rx ) & device->rx_mod_mask;
        rxp = &device->rx_ring[ entry ];
    }

    return npackets;
}

static int pcnet32_tx( pcnet32_private_t* device ) {
    int delta;
    int must_restart;
    uint32_t dirty_tx;

    dirty_tx = device->dirty_tx;
    must_restart = 0;

    while ( dirty_tx != device->cur_tx ) {
        int entry;
        int status;

        entry = dirty_tx & device->tx_mod_mask;
        status = ( short )device->tx_ring[ entry ].status;

        if ( status < 0 ) {
            break;
        }

        device->tx_ring[ entry ].base = 0;

        if ( status & 0x4000 ) {
            int err_status;

            /* There was a major error, log it. */

            err_status = device->tx_ring[ entry ].misc;
            device->stats.tx_errors++;

            kprintf( ERROR, "PCnet32: Tx error status=%04x err_status=%08x\n", status, err_status );

            must_restart = 1;
        } else {
            device->stats.tx_packets++;
        }

        /* We must free the original skb */

        if ( device->tx_packet_buf[ entry ] ) {
            delete_packet( device->tx_packet_buf[ entry ] );
            device->tx_packet_buf[ entry ] = NULL;
        }

        dirty_tx++;
    }

    delta = ( device->cur_tx - dirty_tx ) & ( device->tx_mod_mask + device->tx_ring_size );

    if ( delta > device->tx_ring_size ) {
        kprintf( ERROR, "PCnet32: out-of-sync dirty pointer, %d vs. %d, full=%d.\n", dirty_tx, device->cur_tx, device->tx_full );
        dirty_tx += device->tx_ring_size;
        delta -= device->tx_ring_size;
    }

#if 0
    if (lp->tx_full &&
            netif_queue_stopped(dev) &&
            delta < lp->tx_ring_size - 2) {
                /* The ring is no longer full, clear tbusy. */
                lp->tx_full = 0;
                netif_wake_queue(dev);
    }
#endif

    device->dirty_tx = dirty_tx;

    return must_restart;
}

static int pcnet32_start_xmit( pcnet32_private_t* device, packet_t* packet ) {
    int entry;
    int io_address;
    uint16_t status;

    io_address = device->io_address;

    spinlock_disable( &device->lock );

    /* Default status -- will not enable Successful-TxDone
     * interrupt when that option is available to us.
     */

    status = 0x8300;

    /* Fill in a Tx ring entry */

    /* Mask to ring buffer boundary. */

    entry = device->cur_tx & device->tx_mod_mask;

    /* Caution: the write order is important here, set the status
     * with the "ownership" bits last. */

    device->tx_ring[ entry ].length = -packet->size;
    device->tx_ring[ entry ].misc = 0x00000000;
    device->tx_packet_buf[ entry ] = packet;
    device->tx_ring[ entry ].base = ( uint32_t )packet->data;

    /* Make sure owner changes after all others are visible */

    wmb();

    device->tx_ring[ entry ].status = status;

    device->cur_tx++;
    device->stats.tx_bytes += packet->size;

    /* Trigger an immediate send poll. */

    device->access->write_csr( io_address, CSR0, CSR0_INTEN | CSR0_TXPOLL );

    if ( device->tx_ring[ ( entry + 1 ) & device->tx_mod_mask ].base != 0 ) {
        device->tx_full = 1;
        //netif_stop_queue(dev);
    }

    spinunlock_enable( &device->lock );

    return 0;
}

static int pcnet32_interrupt( int irq, void* data, registers_t* regs ) {
    int boguscnt;
    uint16_t csr0;
    int io_address;
    pcnet32_private_t* device;

    device = ( pcnet32_private_t* )data;
    io_address = device->io_address;

    spinlock_disable( &device->lock );

    boguscnt = max_interrupt_work;
    csr0 = device->access->read_csr( io_address, CSR0 );

    while ( ( csr0 & 0x8F00 ) && ( --boguscnt >= 0 ) ) {
        if ( csr0 == 0xFFFF ) {
            /* PCMCIA remove happened */
            break;
        }

        /* Acknowledge all of the current interrupt sources ASAP. */

        device->access->write_csr( io_address, CSR0, csr0 & ~0x004F );

        /* Log misc errors. */

        if ( csr0 & 0x4000 ) {
            device->stats.tx_errors++; /* Tx babble. */
        }

        if ( csr0 & 0x1000 ) {
            /*
             * This happens when our receive ring is full. This
             * shouldn't be a problem as we will see normal rx
             * interrupts for the frames in the receive ring.  But
             * there are some PCI chipsets (I can reproduce this
             * on SP3G with Intel saturn chipset) which have
             * sometimes problems and will fill up the receive
             * ring with error descriptors.  In this situation we
             * don't get a rx interrupt, but a missed frame
             * interrupt sooner or later.
             */

             pcnet32_rx( device );

             device->stats.rx_errors++; /* Missed a Rx frame. */
        }


        if ( csr0 & 0x0800 ) {
            /* Unlike for the lance, there is no restart needed */

            kprintf( ERROR, "PCnet32: Bus master arbitration failure, status %x.\n", csr0 );
        }

        if ( csr0 & 0x0400 ) {
            pcnet32_rx( device );
        }

        if ( csr0 & 0x0200 ) {
            pcnet32_tx( device );
        }

        csr0 = device->access->read_csr( io_address, CSR0 );
    }

    /* Clear any other interrupt, and set interrupt enable. */

    device->access->write_csr( io_address, 0, 0x7940 );

    spinunlock_enable( &device->lock );

    return 0;
}

static int pcnet32_init_ring( pcnet32_private_t* device ) {
    int i;
    packet_t* packet;

    device->tx_full = 0;
    device->cur_rx = 0;
    device->cur_tx = 0;
    device->dirty_rx = 0;
    device->dirty_tx = 0;

    for ( i = 0; i < device->rx_ring_size; i++ ) {
        packet = device->rx_packet_buf[ i ];

        if ( packet == NULL ) {
            packet = create_packet( PKT_BUF_SKB );

            if ( packet == NULL ) {
                kprintf( ERROR, "PCnet32: create_packet() failed for rx_ring entry!\n" );

                return -ENOMEM;
            }

            device->rx_packet_buf[ i ] = packet;
        }

        rmb();

        device->rx_ring[ i ].base = ( uint32_t )packet->data;
        device->rx_ring[ i ].buf_length = -PKT_BUF_SKB;

        /* Make sure owner changes after all others are visible */

        wmb();

        device->rx_ring[ i ].status = 0x8000;
    }

    /* The Tx buffer address is filled in as needed, but we do need to clear
        the upper ownership bit. */

    for ( i = 0; i < device->tx_ring_size; i++ ) {
        /* CPU owns buffer */

        device->tx_ring[ i ].status = 0;

        /* Make sure adapter sees owner change */

        wmb();

        device->tx_ring[ i ].base = 0;
    }

    device->init_block->tlen_rlen = device->tx_len_bits | device->rx_len_bits;

    for ( i = 0; i < 6; i++ ) {
        device->init_block->phys_addr[ i ] = device->dev_address[ i ];
    }

    device->init_block->rx_ring = ( uint32_t )device->rx_ring;
    device->init_block->tx_ring = ( uint32_t )device->tx_ring;

    /* Make sure all changes are visible */

    wmb();

    return 0;
}

static int pcnet32_start( pcnet32_private_t* device ) {
    int i;
    int error;
    int io_address;
    uint16_t value;

    io_address = device->io_address;

    error = request_irq( device->irq, pcnet32_interrupt, ( void* )device );

    if ( error < 0 ) {
        return error;
    }

    spinlock_disable( &device->lock );

    if ( !is_valid_ether_addr( device->dev_address ) ) {
        goto error_free_irq;
    }

    /* Reset the device */

    device->access->reset( io_address );

    /* Wwitch to 32bit mode */

    device->access->write_bcr( io_address, 20, 2 );

    /* Set/reset autoselect bit */

    value = device->access->read_bcr( io_address, 2 ) & ~2;

    if ( device->options & PCNET32_PORT_ASEL ) {
        value |= 2;
    }

    device->access->write_bcr( io_address, 2, value );

    /* Set/reset GPSI bit in test register */

    value = device->access->read_csr( io_address, 124 ) & ~0x10;

    if ( ( device->options & PCNET32_PORT_PORTSEL ) == PCNET32_PORT_GPSI ) {
        value |= 0x10;
    }

    device->access->write_csr( io_address, 124, value );

    if ( device->phycount < 2 ) {
        /*
         * 24 Jun 2004 according AMD, in order to change the PHY,
         * DANAS (or DISPM for 79C976) must be set; then select the speed,
         * duplex, and/or enable auto negotiation, and clear DANAS
         */

        if ( ( device->mii ) &&
             !( device->options & PCNET32_PORT_ASEL ) ) {
            device->access->write_bcr(
                io_address,
                32,
                device->access->read_bcr( io_address, 32 ) | 0x0080
            );

            /* Disable Auto Negotiation, set 10Mpbs, HD */

            value = device->access->read_bcr( io_address, 32 ) & ~0xB8;

            if ( device->options & PCNET32_PORT_FD ) {
                value |= 0x10;
            }

            if ( device->options & PCNET32_PORT_100 ) {
                value |= 0x08;
            }

            device->access->write_bcr( io_address, 32, value );
        } else {
            if ( device->options & PCNET32_PORT_ASEL ) {
                device->access->write_bcr(
                    io_address,
                    32,
                    device->access->read_bcr( io_address, 32 ) | 0x0080
                );

                /* Enable auto negotiate, setup, disable fd */

                value = device->access->read_bcr( io_address, 32 ) & ~0x98;
                value |= 0x20;
                device->access->write_bcr( io_address, 32, value );
            }
        }
    } else {
        kprintf( WARNING, "PCnet32 TODO: Support multiple PHYs\n" );
    }

    device->init_block->mode = ( ( device->options & PCNET32_PORT_PORTSEL ) << 7 );

    /* pcnet32_load_multicast( device ); */

    if ( pcnet32_init_ring( device ) ) {
        goto error_free_ring;
    }

    /* Re-initialize the PCNET32, and start it when done. */

    device->access->write_csr( io_address, 1, ( uint32_t )device->init_block & 0xFFFF );
    device->access->write_csr( io_address, 2, ( ( uint32_t )device->init_block ) >> 16 );

    device->access->write_csr( io_address, CSR4, 0x0915 );
    device->access->write_csr( io_address, CSR0, CSR0_INIT );

    if ( device->chip_version >= PCNET32_79C970A ) {
        /* TODO pcnet32_check_media(dev, 1); */
    }

    i = 0;

    while ( i++ < 100 ) {
        if ( device->access->read_csr( io_address, CSR0 ) & CSR0_IDON ) {
            break;
        }
    }

    /*
     * We used to clear the InitDone bit, 0x0100, here but Mark Stockton
     * reports that doing so triggers a bug in the '974.
     */

    device->access->write_csr( io_address, CSR0, CSR0_NORMAL );

    spinunlock_enable( &device->lock );

    return 0;

error_free_ring:
    spinunlock_enable( &device->lock );

error_free_irq:
    /* TODO: release IRQ */

    return -ENOMEM;
}

static int pcnet32_open( void* node, uint32_t flags, void** cookie ) {
    return 0;
}

static int pcnet32_close( void* node, void* cookie ) {
    return 0;
}

static int pcnet32_ioctl( void* node, void* cookie, uint32_t command, void* args, bool from_kernel ) {
    int error;
    pcnet32_private_t* device;

    device = ( pcnet32_private_t* )node;

    switch ( command ) {
        case IOCTL_NET_SET_IN_QUEUE :
            device->input_queue = ( packet_queue_t* )args;
            error = 0;
            break;

        case IOCTL_NET_START :
            error = pcnet32_start( device );
            break;

        case IOCTL_NET_GET_HW_ADDRESS :
            memcpy( args, device->dev_address, ETH_ADDR_LEN );
            error = 0;
            break;

        default :
            error = -ENOSYS;
            break;
    }

    return error;
}

static int pcnet32_write( void* node, void* cookie, const void* buffer, off_t position, size_t size ) {
    packet_t* packet;
    pcnet32_private_t* device;

    device = ( pcnet32_private_t* )node;

    packet = create_packet( size );

    if ( packet == NULL ) {
        return -ENOMEM;
    }

    memcpy( packet->data, buffer, size );

    pcnet32_start_xmit( device, packet );

    return size;
}

static device_calls_t pcnet32_calls = {
    .open = pcnet32_open,
    .close = pcnet32_close,
    .ioctl = pcnet32_ioctl,
    .read = NULL,
    .write = pcnet32_write,
    .add_select_request = NULL,
    .remove_select_request = NULL
};

static int pcnet32_do_probe( pci_device_t* pci_device ) {
    int i;
    int fdx;
    int mii;
    int fset;
    int dxsuflo;
    int error;
    char path[ 128 ];
    int chip_version;
    char* chip_name;
    uint32_t tmp;
    uint32_t io_addr;
    uint8_t prom_addr[ 6 ];
    pcnet32_access_t* access = NULL;
    pcnet32_private_t* device;

    io_addr = pci_device->base[ 0 ] & PCI_ADDRESS_IO_MASK;

    kprintf( INFO, "PCnet32: I/O address: 0x%X\n", io_addr );

    /* Reset the chip */

    pcnet32_wio_reset( io_addr );

    /* 16-bit check is first, otherwise some older PCnet chips fail */

    if ( ( pcnet32_wio_read_csr( io_addr, 0 ) == 4 ) &&
         ( pcnet32_wio_check( io_addr ) ) ) {
        access = &pcnet32_wio;
    } else {
        pcnet32_dwio_reset( io_addr );

        if ( ( pcnet32_dwio_read_csr( io_addr, 0 ) == 4 ) &&
             ( pcnet32_dwio_check( io_addr ) ) ) {
            access = &pcnet32_dwio;
        } else {
            return -ENODEV;
        }
    }

    chip_version = access->read_csr( io_addr, 88 ) | ( access->read_csr( io_addr, 89 ) << 16 );

    if ( ( chip_version & 0xFFF ) != 0x003 ) {
        kprintf( WARNING, "PCnet32: Unsupported chip version.\n" );
        return -ENODEV;
    }

    chip_version = ( chip_version >> 12 ) & 0xFFFF;

    fdx = 0;
    mii = 0;
    fset = 0;
    dxsuflo = 0;

    switch ( chip_version ) {
        case 0x2420 :
            chip_name = "PCnet/PCI 79C970";
            break;

        case 0x2430 :
            if ( /*shared*/ 0 ) {
                chip_name = "PCnet/PCI 79C970";
            } else {
                chip_name = "PCnet/32 79C965";
            }

            break;

        case 0x2621 :
            chip_name = "PCnet/PCI II 79C970A";
            fdx = 1;
            break;

        case 0x2623 :
            chip_name = "PCnet/FAST 79C971";
            fdx = 1;
            mii = 1;
            fset = 1;
            break;

        case 0x2624 :
            chip_name = "PCnet/FAST+ 79C972";
            fdx = 1;
            mii = 1;
            fset = 1;
            break;

        case 0x2625 :
            chip_name = "PCnet/FAST III 79C973";
            fdx = 1;
            mii = 1;
            break;

        case 0x2626 : {
            int media;

            chip_name = "PCnet/Home 79C978";
            fdx = 1;

            /*
             * This is based on specs published at www.amd.com.  This section
             * assumes that a card with a 79C978 wants to go into standard
             * ethernet mode.  The 79C978 can also go into 1Mb HomePNA mode,
             * and the module option homepna=1 can select this instead.
             */

            media = access->read_bcr( io_addr, 49 );
            media &= ~3; /* default to 10Mb ethernet */
#if 0
                if (cards_found < MAX_UNITS && homepna[cards_found])
                        media |= 1;     /* switch to home wiring mode */
                if (pcnet32_debug & NETIF_MSG_PROBE)
                        printk(KERN_DEBUG PFX "media set to %sMbit mode.\n",
                               (media & 1) ? "1" : "10");
#endif
            access->write_bcr( io_addr, 49, media );

            break;
        }

        case 0x2627 :
            chip_name = "PCnet/FAST III 79C975";
            fdx = 1;
            mii = 1;
            break;

        case 0x2628 :
            chip_name = "PCnet/PRO 79C976";
            fdx = 1;
            mii = 1;
            break;

        default:
            kprintf( WARNING, "PCnet32: Unknown chip version.\n" );
            return -ENODEV;
    }

    kprintf( INFO, "PCnet32: Found %s chip\n", chip_name );

    /*
     *  On selected chips turn on the BCR18:NOUFLO bit. This stops transmit
     *  starting until the packet is loaded. Strike one for reliability, lose
     *  one for latency - although on PCI this isnt a big loss. Older chips
     *  have FIFO's smaller than a packet, so you can't do this.
     *  Turn on BCR18:BurstRdEn and BCR18:BurstWrEn.
     */

    if ( fset ) {
        access->write_bcr( io_addr, 18, ( access->read_bcr( io_addr, 18 ) | 0x0860 ) );
        access->write_csr( io_addr, 80, ( access->read_csr( io_addr, 80 ) & 0x0C00 ) | 0x0C00 );
        dxsuflo = 1;
    }

    device = ( pcnet32_private_t* )kmalloc( sizeof( pcnet32_private_t ) );

    if ( device == NULL ) {
        return -ENOMEM;
    }

    /* In most chips, after a chip reset, the ethernet address is read from the
     * station address PROM at the base address and programmed into the
     * "Physical Address Registers" CSR12-14.
     * As a precautionary measure, we read the PROM values and complain if
     * they disagree with the CSRs.  If they miscompare, and the PROM addr
     * is valid, then the PROM addr is used.
     */

    for ( i = 0; i < 3; i++ ) {
        unsigned int value;

        value = access->read_csr( io_addr, i + 12 ) & 0x0FFFF;
        /* There may be endianness issues here. */
        device->dev_address[ 2 * i ] = value & 0x0FF;
        device->dev_address[ 2 * i + 1 ] = ( value >> 8 ) & 0x0FF;
    }

    /* Read PROM address and compare with CSR address */

    for ( i = 0; i < 6; i++ ) {
        prom_addr[ i ] = inb( io_addr + i );
    }

    if ( ( memcmp( prom_addr, device->dev_address, ETH_ADDR_LEN ) ) ||
         ( !is_valid_ether_addr( device->dev_address ) ) ) {
        if ( is_valid_ether_addr( prom_addr ) ) {
            kprintf( WARNING, "PCnet32: CSR address invalid, using PROM address instead\n" );
            memcpy( device->dev_address, prom_addr, ETH_ADDR_LEN );
        }
    }

    memcpy( device->perm_address, device->dev_address, ETH_ADDR_LEN );

    if ( !is_valid_ether_addr( device->perm_address ) ) {
        memset( device->dev_address, 0, sizeof( device->dev_address ) );
    }

    kprintf(
        INFO,
        "PCnet32: MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
        device->dev_address[ 0 ],
        device->dev_address[ 1 ],
        device->dev_address[ 2 ],
        device->dev_address[ 3 ],
        device->dev_address[ 4 ],
        device->dev_address[ 5 ]
    );

    /* Allocate init block */

    device->init_block_base = kmalloc( sizeof( pcnet32_init_block_t ) + 15 );

    if ( device->init_block_base == NULL ) {
        /* TODO: free device? */
        return -ENOMEM;
    }

    tmp = ( uint32_t )device->init_block_base;
    tmp = ( tmp + 15 ) & ~15;
    device->init_block = ( pcnet32_init_block_t* )tmp;

    init_spinlock( &device->lock, "pcnet32 device" );

    device->access = access;
    device->io_address = io_addr;
    device->chip_version = chip_version;
    device->tx_ring_size = TX_RING_SIZE;
    device->rx_ring_size = RX_RING_SIZE;
    device->tx_mod_mask = device->tx_ring_size - 1;
    device->rx_mod_mask = device->rx_ring_size - 1;
    device->tx_len_bits = ( PCNET32_LOG_TX_BUFFERS << 12 );
    device->rx_len_bits = ( PCNET32_LOG_RX_BUFFERS << 4 );
    device->mii = mii;
    device->phymask = 0;
    device->phycount = 0;
    device->mii_if.full_duplex = fdx;
    device->mii_if.phy_id_mask = 0x1F;
    device->mii_if.reg_num_mask = 0x1F;
    device->options = PCNET32_PORT_ASEL;
    device->input_queue = NULL;
    memset( &device->stats, 0, sizeof( net_device_stats_t ) );

    error = pcnet32_alloc_ring( device );

    if ( error < 0 ) {
        /* TODO: cleanup */
        return error;
    }

    if ( ( device->dev_address[ 0 ] == 0x00 ) &&
         ( device->dev_address[ 1 ] == 0xE0 ) &&
         ( device->dev_address[ 2 ] == 0x75 ) ) {
        device->options = PCNET32_PORT_FD | PCNET32_PORT_GPSI;
    }

    device->init_block->mode = 0x0003; /* Disable Rx and Tx. */
    device->init_block->tlen_rlen = device->tx_len_bits | device->rx_len_bits;

    for ( i = 0; i < 6; i++ ) {
        device->init_block->phys_addr[ i ] = device->dev_address[ i ];
    }

    device->init_block->filter[ 0 ] = 0x00000000;
    device->init_block->filter[ 1 ] = 0x00000000;
    device->init_block->rx_ring = ( uint32_t )device->rx_ring;
    device->init_block->tx_ring = ( uint32_t )device->tx_ring;

    /* Switch pcnet32 to 32bit mode */

    access->write_bcr( io_addr, 20, 2 );

    access->write_csr( io_addr, 1, ( ( uint32_t )device->init_block & 0xFFFF ) );
    access->write_csr( io_addr, 2, ( ( uint32_t )device->init_block >> 16 ) );

    /* Set the mii phy_id so that we can query the link state */

    if ( device->mii ) {
        device->mii_if.phy_id = ( ( device->access->read_bcr( io_addr, 33 ) ) >> 5 ) & 0x1F;

        /* Scan for PHYs */

        for ( i = 0; i < PCNET32_MAX_PHYS; i++ ) {
            uint16_t id1;
            uint16_t id2;

            id1 = mdio_read( device, i, MII_PHYSID1 );

            if ( id1 == 0xFFFF ) {
                continue;
            }

            id2 = mdio_read( device, i, MII_PHYSID2 );

            if (id2 == 0xFFFF ) {
                continue;
            }

            if ( ( i == 31 ) &&
                 ( ( chip_version + 1 ) & 0xFFFE ) == 0x2624 ) {
                /* 79C971 & 79C972 have phantom phy at id 31 */
                continue;
            }

            device->phycount++;
            device->phymask |= ( 1 << i );
            device->mii_if.phy_id = i;

            kprintf( INFO, "PCnet32: Found PHY %04x:%04x at address %d.\n", id1, id2, i );
        }

        device->access->write_bcr( io_addr, 33, ( device->mii_if.phy_id ) << 5 );

        if ( device->phycount > 1 ) {
            device->options |= PCNET32_PORT_MII;
        }
    }

    device->irq = pci_device->interrupt_line;

    if ( device->irq < 2 ) {
        /* TODO: cleanup */
        return -EINVAL;
    }

    kprintf( INFO, "PCnet32: Assigned IRQ %d.\n", device->irq );

    /* Register pcnet32 device */

    snprintf( path, sizeof( path ), "network/pcnet32_%d", cards_found );

    error = create_device_node( path, &pcnet32_calls, ( void* )device );

    if ( error < 0 ) {
        /* TODO: cleanup */
        return error;
    }

    /* Enable LED writes */

    access->write_bcr( io_addr, 2, access->read_bcr( io_addr, 2 ) | 0x1000 );

    return 0;
}

static int pcnet32_probe( pci_bus_t* pci_bus, pci_device_t* pci_device ) {
    int error;
    uint32_t pci_command;

    /* Set master */

    error = pci_bus->read_config( pci_device, PCI_COMMAND, 2, &pci_command );

    if ( error < 0 ) {
        return error;
    }

    if ( ( pci_command & PCI_COMMAND_MASTER ) == 0 ) {
        kprintf( INFO, "PCnet32: PCI master bit not set, setting it ...\n" );

        pci_command |= ( PCI_COMMAND_MASTER | PCI_COMMAND_IO );

        error = pci_bus->write_config( pci_device, PCI_COMMAND, 2, pci_command );

        if ( error < 0 ) {
            return error;
        }
    }

    return pcnet32_do_probe( pci_device );
}

int init_module( void ) {
    int i;
    int j;
    int error;
    int device_count;
    pci_bus_t* pci_bus;
    pci_device_t* pci_device;

    pci_bus = get_bus_driver( "PCI" );

    if ( pci_bus == NULL ) {
        kprintf( WARNING, "PATA: PCI bus not found!\n" );
        return -1;
    }

    device_count = pci_bus->get_device_count();

    for ( i = 0; i < device_count; i++ ) {
        pci_device = pci_bus->get_device( i );

        for ( j = 0; j < ARRAY_SIZE( pci_table ); j++ ) {
            if ( ( pci_table[ j ].vendor_id == pci_device->vendor_id ) &&
                 ( pci_table[ j ].device_id == pci_device->device_id ) ) {
                error = pcnet32_probe( pci_bus, pci_device );

                if ( error >= 0 ) {
                    cards_found++;
                } else {
                    kprintf(
                        ERROR,
                        "PCnet32: Failed to initialize card at %d:%d:%d\n",
                        pci_device->bus,
                        pci_device->dev,
                        pci_device->func
                    );
                }

                break;
            }
        }
    }

    return 0;
}

int destroy_module( void ) {
    return 0;
}
