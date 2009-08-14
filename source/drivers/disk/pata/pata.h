/* Parallel AT Attachment driver
 *
 * Copyright (c) 2008 Zoltan Kovacs
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

#ifndef _PATA_H_
#define _PATA_H_

#include <types.h>
#include "../../bus/pci/pci.h"
#include <lock/mutex.h>

/* Maximum number of ports per controller */

#define MAX_PATA_PORTS 4

/* PATA registers */

#define PATA_REG_DATA     0
#define PATA_REG_ERROR    1
#define PATA_REG_FEATURE  1
#define PATA_REG_COUNT    2
#define PATA_REG_LBA_LOW  3
#define PATA_REG_LBA_MID  4
#define PATA_REG_LBA_HIGH 5
#define PATA_REG_DEVICE   6
#define PATA_REG_STATUS   7
#define PATA_REG_COMMAND  7
#define PATA_REG_CONTROL  8

/* PATA commands */

#define PATA_CMD_READ_SECTORS           0x20
#define PATA_CMD_READ_SECTORS_EXT       0x24
#define PATA_CMD_WRITE_SECTORS          0x30
#define PATA_CMD_WRITE_SECTORS_EXT      0x34
#define PATA_CMD_PACKET                 0xA0
#define PATA_CMD_IDENTIFY_PACKET_DEVICE 0xA1
#define PATA_CMD_IDENTIFY_DEVICE        0xEC

/* ATAPI commands */

#define ATAPI_CMD_READ_CAPACITY 0x25
#define ATAPI_CMD_READ_10       0x28

/* Possible bits in the status register */

#define PATA_STATUS_ERROR 0x01
#define PATA_STATUS_DRQ   0x08
#define PATA_STATUS_DRDY  0x40
#define PATA_STATUS_BUSY  0x80

/* Possible bits in the control register */

#define PATA_CONTROL_INTDISABLE 0x02
#define PATA_CONTROL_SW_RESET   0x04
#define PATA_CONTROL_DEFAULT    0x08

/* Possible bits in the device register */

#define PATA_DEVICE_LBA 0x40

#define PATA_SELECT_DEFAULT 0xE0
#define PATA_SELECT_SLAVE   0x10

typedef struct pata_identify_info {
    uint16_t configuration;
    uint16_t cylinders;
    uint16_t reserved1;
    uint16_t heads;
    uint16_t track_bytes;
    uint16_t sector_bytes;
    uint16_t sectors;
    uint16_t reserved2[ 3 ];
    uint8_t serial_number[ 20 ];
    uint16_t buf_type;
    uint16_t buf_size;
    uint16_t ecc_bytes;
    uint8_t revision[ 8 ];
    uint8_t model_id[ 40 ];
    uint8_t sectors_per_rw_long;
    uint8_t reserved3;
    uint16_t reserved4;
    uint8_t reserved5;
    uint8_t capabilities;
    uint16_t reserved6;
    uint8_t reserved7;
    uint8_t pio_cycle_time;
    uint8_t reserved8;
    uint8_t dma;
    uint16_t valid;
    uint16_t current_cylinders;
    uint16_t current_heads;
    uint16_t current_sectors;
    uint16_t current_capacity0;
    uint16_t current_capacity1;
    uint8_t sectors_per_rw_irq;
    uint8_t sectors_per_rw_irq_valid;
    uint32_t lba_sectors;
    uint16_t single_word_dma_info;
    uint16_t multi_word_dma_info;
    uint16_t eide_pio_modes;
    uint16_t eide_dma_min;
    uint16_t eide_dma_time;
    uint16_t eide_pio;
    uint16_t eide_pio_iordy;
    uint16_t reserved9[ 2 ];
    uint16_t reserved10[ 4 ];
    uint16_t command_queue_depth;
    uint16_t reserved11[ 4 ];
    uint16_t major;
    uint16_t minor;
    uint16_t command_set_1;
    uint16_t command_set_2;
    uint16_t command_set_features_extensions;
    uint16_t command_set_features_enable_1;
    uint16_t command_set_features_enable_2;
    uint16_t command_set_features_default;
    uint16_t ultra_dma_modes;
    uint16_t reserved12[ 2 ];
    uint16_t advanced_power_management;
    uint16_t reserved13;
    uint16_t hardware_config;
    uint16_t acoustic;
    uint16_t reserved14[ 5 ];
    uint64_t lba_capacity_48;
    uint16_t reserved15[ 22 ];
    uint16_t last_lun;
    uint16_t reserved16;
    uint16_t device_lock_functions;
    uint16_t current_set_features_options;
    uint16_t reserved17[ 26 ];
    uint16_t reserved18;
    uint16_t reserved19[ 3 ];
    uint16_t reserved20[ 96 ];
} pata_identify_info_t;

typedef struct pata_port {
    bool present;

    int channel;
    bool is_slave;

    bool is_atapi;
    uint16_t cmd_base;
    uint16_t ctrl_base;

    bool use_lba;
    bool use_lba48;
    uint64_t capacity;
    uint32_t sector_size;

    bool open;
    lock_id mutex;

    char model_name[ 41 ];
    pata_identify_info_t identify_info;
} pata_port_t;

typedef struct pata_controller {
    int channels;
    int ports_per_channel;
    pata_port_t* ports[ MAX_PATA_PORTS ];
    pci_device_t pci_device;
    struct pata_controller* next;
} pata_controller_t;

extern int controller_count;
extern pata_controller_t* controllers;

/* Controller functions */

int pata_detect_controllers( void );
int pata_initialize_controller( pata_controller_t* controller );

/* Port functions */

int pata_port_wait( pata_port_t* port, uint8_t set, uint8_t clear, bool check_error, uint64_t timeout );
void pata_port_select( pata_port_t* port );
bool pata_is_port_present( pata_port_t* port );
int pata_port_identify( pata_port_t* port );
int pata_configure_ata_port( pata_port_t* port );
int pata_configure_atapi_port( pata_port_t* port );

/* Command control functions */

int pata_port_send_command( pata_port_t* port, uint8_t cmd, bool check_drdy, uint8_t* regs, uint8_t reg_mask );
int pata_port_finish_command( pata_port_t* port, bool busy_wait, bool check_drdy, uint64_t timeout );
int pata_port_ata_read( pata_port_t* port, void* buffer, uint64_t offset, size_t size );
int pata_port_ata_write( pata_port_t* port, void* buffer, uint64_t offset, size_t size );
int pata_port_atapi_do_packet( pata_port_t* port, uint8_t* packet, bool do_read, void* buffer, size_t size );

/* PIO functions */

void pata_port_read_pio( pata_port_t* port, void* buffer, size_t word_count );
void pata_port_write_pio( pata_port_t* port, void* buffer, size_t word_count );

/* Disk and CDROM functions */

int pata_create_ata_device_node( pata_port_t* port );
int pata_create_atapi_device_node( pata_port_t* port );

#endif // _PATA_H_
