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

#ifndef _PCNET32_H_
#define _PCNET32_H_

#include <types.h>
#include <network/packet.h>
#include <network/mii.h>
#include <network/device.h>
#include <network/ethernet.h>

#include <arch/spinlock.h>

#define PCI_VENDOR_ID_AMD 0x1022

#define PCI_DEVICE_ID_AMD_LANCE      0x2000
#define PCI_DEVICE_ID_AMD_LANCE_HOME 0x2001

#define PCNET32_WIO_RDP   0x10
#define PCNET32_WIO_RAP   0x12
#define PCNET32_WIO_RESET 0x14
#define PCNET32_WIO_BDP   0x16

#define PCNET32_DWIO_RDP   0x10
#define PCNET32_DWIO_RAP   0x14
#define PCNET32_DWIO_RESET 0x18
#define PCNET32_DWIO_BDP   0x1C

#define PCNET32_PORT_AUI     0x00
#define PCNET32_PORT_10BT    0x01
#define PCNET32_PORT_GPSI    0x02
#define PCNET32_PORT_MII     0x03
#define PCNET32_PORT_PORTSEL 0x03
#define PCNET32_PORT_ASEL    0x04
#define PCNET32_PORT_100     0x40
#define PCNET32_PORT_FD      0x80

#define PCNET32_LOG_TX_BUFFERS 4
#define PCNET32_LOG_RX_BUFFERS 5

#define PCNET32_MAX_PHYS 32

#define TX_RING_SIZE ( 1 << PCNET32_LOG_TX_BUFFERS )
#define RX_RING_SIZE ( 1 << PCNET32_LOG_RX_BUFFERS )

#define CSR0            0
#define CSR0_INIT       0x1
#define CSR0_START      0x2
#define CSR0_STOP       0x4
#define CSR0_TXPOLL     0x8
#define CSR0_INTEN      0x40
#define CSR0_IDON       0x0100
#define CSR0_NORMAL     (CSR0_START | CSR0_INTEN)
#define CSR3            3
#define CSR4            4
#define CSR5            5
#define CSR5_SUSPEND    0x0001
#define CSR15           15

#define PCNET32_79C970A 0x2621

typedef struct pcnet32_pci_entry {
    uint16_t vendor_id;
    uint16_t device_id;
} pcnet32_pci_entry_t;

typedef struct pcnet32_access {
    uint16_t ( *read_csr )( uint32_t , int );
    void ( *write_csr )( uint32_t, int, uint16_t );
    uint16_t ( *read_bcr )( uint32_t, int );
    void ( *write_bcr ) ( uint32_t, int, uint16_t );
    uint16_t ( *read_rap )( uint32_t );
    void ( *write_rap )( uint32_t, uint16_t );
    void ( *reset )( uint32_t );
} pcnet32_access_t;

typedef struct pcnet32_init_block {
    uint16_t mode;
    uint16_t tlen_rlen;
    uint8_t phys_addr[ 6 ];
    uint16_t reserved;
    uint32_t filter[ 2 ];
    uint32_t rx_ring;
    uint32_t tx_ring;
} __attribute__(( packed )) pcnet32_init_block_t;

typedef struct pcnet32_rx_head {
    uint32_t base;
    uint16_t buf_length;
    uint16_t status;
    uint32_t msg_length;
    uint32_t reserved;
} __attribute__(( packed )) pcnet32_rx_head_t;

typedef struct pcnet32_tx_head {
    uint32_t base;
    uint16_t length;
    uint16_t status;
    uint32_t misc;
    uint32_t reserved;
} __attribute__(( packed )) pcnet32_tx_head_t;

typedef struct pcnet32_private {
    spinlock_t lock;

    int irq;
    int io_address;
    int chip_version;
    pcnet32_access_t* access;
    uint32_t rx_ring_size;
    uint32_t tx_ring_size;
    uint32_t rx_mod_mask;
    uint32_t tx_mod_mask;
    uint16_t rx_len_bits;
    uint16_t tx_len_bits;
    char tx_full;
    uint32_t cur_tx;
    uint32_t cur_rx;
    uint32_t dirty_tx;
    uint32_t dirty_rx;
    uint32_t options;
    uint8_t dev_address[ ETH_ADDR_LEN ];
    uint8_t perm_address[ ETH_ADDR_LEN ];

    int mii;
    uint32_t phymask;
    int phycount;
    mii_if_info_t mii_if;

    void* init_block_base;
    pcnet32_init_block_t* init_block;

    void* rx_ring_base;
    void* tx_ring_base;
    pcnet32_rx_head_t* rx_ring;
    pcnet32_tx_head_t* tx_ring;
    packet_t** rx_packet_buf;
    packet_t** tx_packet_buf;

    packet_queue_t* input_queue;
    net_device_stats_t stats;
} pcnet32_private_t;

#endif /* _PCNET32_H_ */
