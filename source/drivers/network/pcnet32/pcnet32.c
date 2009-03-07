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
#include <mm/kmalloc.h>
#include <vfs/devfs.h>
#include <lib/string.h>

#include <arch/io.h>

#include "../../bus/pci/pci.h"

#include "pcnet32.h"

static int cards_found = 0;

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

    tmp = ( uint32_t )device->tx_ring_base;
    tmp = ( tmp + 15 ) & ~15;
    device->tx_ring = ( pcnet32_tx_head_t* )tmp;

    tmp = ( uint32_t )device->rx_ring_base;
    tmp = ( tmp + 15 ) & ~15;
    device->rx_ring = ( pcnet32_rx_head_t* )tmp;

    return 0;
}

static device_calls_t pcnet32_calls = {
    .open = NULL,
    .close = NULL,
    .read = NULL,
    .write = NULL,
    .ioctl = NULL,
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

    kprintf( "PCnet32: I/O address: 0x%X\n", io_addr );

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
        kprintf( "PCnet32: Unsupported chip version.\n" );
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
            kprintf( "PCnet32: Unknown chip version.\n" );
            return -ENODEV;
    }

    kprintf( "PCnet32: Found %s chip\n", chip_name );

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

    if ( ( memcmp( prom_addr, device->dev_address, 6 ) ) ||
         ( !is_valid_ether_addr( device->dev_address ) ) ) {
        if ( is_valid_ether_addr( prom_addr ) ) {
            kprintf( "PCnet32: CSR address invalid, using PROM address instead\n" );
            memcpy( device->dev_address, prom_addr, 6 );
        }
    }

    memcpy( device->perm_address, device->dev_address, /* TODO device->addr_len*/ 6 );

    if ( !is_valid_ether_addr( device->perm_address ) ) {
        memset( device->dev_address, 0, sizeof( device->dev_address ) );
    }

    kprintf(
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
    device->chip_version = chip_version;
    device->tx_ring_size = TX_RING_SIZE;
    device->rx_ring_size = RX_RING_SIZE;
    device->tx_mod_mask = device->tx_ring_size - 1;
    device->rx_mod_mask = device->rx_ring_size - 1;
    device->tx_len_bits = ( PCNET32_LOG_TX_BUFFERS << 12 );
    device->rx_len_bits = ( PCNET32_LOG_RX_BUFFERS << 4 );
    device->options = PCNET32_PORT_ASEL;

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

    device->irq = pci_device->interrupt_line;

    if ( device->irq < 2 ) {
        /* TODO: cleanup */
        return -EINVAL;
    }

    kprintf( "PCnet32: Assigned IRQ %d.\n", device->irq );

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
        kprintf( "PCnet32: PCI master bit not set, setting it ...\n" );

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
        kprintf( "PATA: PCI bus not found!\n" );
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
