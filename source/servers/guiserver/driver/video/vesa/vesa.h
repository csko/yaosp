/* VESA graphics driver
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

#ifndef _VESA_H_
#define _VESA_H_

#include <sys/types.h>

#include <graphicsdriver.h>

typedef struct vesa_info {
    char signature[ 4 ];
    uint16_t version;
    uint32_t oem_string_ptr;
    uint32_t capabilities;
    uint32_t video_mode_ptr;
    uint16_t total_memory;
} __attribute__(( packed )) vesa_info_t;

typedef struct vesa_mode_info {
    uint16_t mode_attributes;
    uint8_t  wina_attributes;
    uint8_t  winb_attributes;
    uint16_t win_granularity;
    uint16_t win_size;
    uint16_t wina_segment;
    uint16_t winb_segment;
    uint32_t win_pos_func_ptr;
    uint16_t bytes_per_scan_line;
    uint16_t width;
    uint16_t height;
    uint8_t  char_width;
    uint8_t  char_height;
    uint8_t  num_planes;
    uint8_t  bits_per_pixel;
    uint8_t  num_banks;
    uint8_t  memory_model_type;
    uint8_t  bank_size;
    uint8_t  num_image_pages;
    uint8_t  reserved1;
    uint8_t  red_mask_size;
    uint8_t  red_field_position;
    uint8_t  green_mask_size;
    uint8_t  green_field_position;
    uint8_t  blue_mask_size;
    uint8_t  blue_field_position;
    uint8_t  reserved_mask_size;
    uint8_t  reserved_mask_position;
    uint8_t  direct_color_mode_info;
    uint32_t phys_base_ptr;
    uint32_t offscreen_mem_ptr;
    uint16_t offscreen_mem_size;
    uint16_t lin_bytes_per_scan_line;
    uint8_t  bnk_num_image_pages;
    uint8_t  lin_num_image_pages;
    uint8_t  lin_red_mask_size;
    uint8_t  lin_red_field_position;
    uint8_t  lin_green_mask_size;
    uint8_t  lin_green_field_position;
    uint8_t  lin_blue_mask_size;
    uint8_t  lin_blue_field_position;
    uint8_t  lin_rsvd_mask_size;
    uint8_t  lin_rsvd_field_position;
    uint32_t  max_pixel_clock;
    uint8_t  reserved4[ 190 ];
} __attribute__(( packed )) vesa_mode_info_t;

typedef struct vesa_cmd_modelist {
    uint32_t max_count;
    uint32_t current_count;
    void* mode_list;
} vesa_cmd_modelist_t;

typedef struct vesa_cmd_modeinfo {
    uint16_t mode_number;
    vesa_mode_info_t mode_info;
} vesa_cmd_modeinfo_t;

typedef struct vesa_cmd_setmode {
    uint16_t mode_number;
} vesa_cmd_setmode_t;

typedef struct vesa_screen_mode {
    screen_mode_t screen_mode;
    uint16_t vesa_mode_id;
    void* phys_base_ptr;
} vesa_screen_mode_t;

extern graphics_driver_t vesa_graphics_driver;

#endif /* _VESA_H_ */
