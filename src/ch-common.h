/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef CH_COMMON_H
#define CH_COMMON_H

/* device constants */
#define	CH_USB_VID				0x04d8
#define	CH_USB_PID				0xf8da
#define	CH_USB_CONFIG				0x0001
#define	CH_USB_INTERFACE			0x0000
#define	CH_USB_HID_EP				0x0001
#define	CH_USB_HID_EP_IN			(CH_USB_HID_EP | 0x80)
#define	CH_USB_HID_EP_OUT			(CH_USB_HID_EP | 0x00)
#define	CH_USB_HID_EP_SIZE			64

/**
 * CH_CMD_GET_COLOR_SELECT:
 *
 * Get the color select state.
 *
 * IN:  [1:cmd]
 * OUT: [1:retval][1:cmd][1:color_select]
 **/
#define	CH_CMD_GET_COLOR_SELECT			0x01

/**
 * CH_CMD_SET_COLOR_SELECT:
 *
 * Set the color select state.
 *
 * IN:  [1:cmd][1:color_select]
 * OUT: [1:retval][1:cmd]
 **/
#define	CH_CMD_SET_COLOR_SELECT			0x02

/**
 * CH_CMD_GET_MULTIPLIER:
 *
 * Gets the multiplier value.
 *
 * IN:  [1:cmd]
 * OUT: [1:retval][1:cmd][1:multiplier_value]
 **/
#define	CH_CMD_GET_MULTIPLIER			0x03

/**
 * CH_CMD_SET_MULTIPLIER:
 *
 * Sets the multiplier value.
 *
 * IN:  [1:cmd][1:multiplier_value]
 * OUT: [1:retval][1:cmd]
 **/
#define	CH_CMD_SET_MULTIPLIER			0x04

/**
 * CH_CMD_GET_INTEGRAL_TIME:
 *
 * Gets the integral time.
 *
 * IN:  [1:cmd]
 * OUT: [1:retval][1:cmd][2:integral_time]
 **/
#define	CH_CMD_GET_INTEGRAL_TIME		0x05

/**
 * CH_CMD_SET_INTEGRAL_TIME:
 *
 * Sets the integral time.
 *
 * IN:  [1:cmd][2:integral_time]
 * OUT: [1:retval][1:cmd]
 **/
#define	CH_CMD_SET_INTEGRAL_TIME		0x06

/**
 * CH_CMD_GET_FIRMWARE_VERSION:
 *
 * Gets the firmware version.
 *
 * IN:  [1:cmd]
 * OUT: [1:retval][1:cmd][2:major][2:minor][2:micro]
 **/
#define	CH_CMD_GET_FIRMWARE_VERSION		0x07

/**
 * CH_CMD_GET_CALIBRATION:
 *
 * Gets the calibration matrix. The description does not have to be NULL terminated.
 *
 * IN:  [1:cmd][2:index]
 * OUT: [1:retval][1:cmd][2*9:matrix_value][24:description]
 **/
#define	CH_CMD_GET_CALIBRATION			0x09

/**
 * CH_CMD_SET_CALIBRATION:
 *
 * Sets the calibration matrix. The @matrix_value parameter is a 16 bit
 * _signed_ value that scales from -1.0 to +1.0.
 *
 * IN:  [1:cmd][2:index][4*9:matrix_value][24:description]
 * OUT: [1:retval][1:cmd]
 **/
#define	CH_CMD_SET_CALIBRATION			0x0a

/**
 * CH_CMD_GET_SERIAL_NUMBER:
 *
 * Gets the device serial number.
 *
 * IN:  [1:cmd]
 * OUT: [1:retval][1:cmd][4:serial_number]
 **/
#define	CH_CMD_GET_SERIAL_NUMBER		0x0b

/**
 * CH_CMD_SET_SERIAL_NUMBER:
 *
 * Sets the device serial number.
 *
 * IN:  [1:cmd][4:serial_number]
 * OUT: [1:retval][1:cmd]
 **/
#define	CH_CMD_SET_SERIAL_NUMBER		0x0c

/**
 * CH_CMD_GET_LEDS:
 *
 * Get the LED state.
 *
 * IN:  [1:cmd]
 * OUT: [1:retval][1:cmd][1:led_state]
 **/
#define	CH_CMD_GET_LEDS				0x0d

/**
 * CH_CMD_SET_LEDS:
 *
 * Set the LED state. Using a @repeat value of anything other than
 * 0 will block the processor for the duration of the command.
 *
 * If @repeat is not 0, then the LEDs are reset to all off at the end
 * of the sequence.
 *
 * IN:  [1:cmd][1:led_state][1:repeat][1:on-time][1:off-time]
 * OUT: [1:retval][1:cmd]
 **/
#define	CH_CMD_SET_LEDS				0x0e

/**
 * CH_CMD_GET_DARK_OFFSETS:
 *
 * Get the dark offsets.
 *
 * IN:  [1:cmd]
 * OUT: [1:retval][1:cmd][2:red][2:green][2:blue]
 **/
#define	CH_CMD_GET_DARK_OFFSETS			0x0f

/**
 * CH_CMD_SET_DARK_OFFSETS:
 *
 * Set the dark offsets.
 *
 * IN:  [1:cmd][2:red][2:green][2:blue]
 * OUT: [1:retval][1:cmd]
 **/
#define	CH_CMD_SET_DARK_OFFSETS			0x10

/**
 * CH_CMD_WRITE_EEPROM:
 *
 * Write values to EEPROM.
 *
 * IN:  [1:cmd][8:eeprom_magic]
 * OUT: [1:retval][1:cmd]
 **/
#define	CH_CMD_WRITE_EEPROM			0x20

/**
 * CH_CMD_TAKE_READING_RAW:
 *
 * Take a raw reading.
 *
 * IN:  [1:cmd]
 * OUT: [1:retval][1:cmd][2:count]
 **/
#define	CH_CMD_TAKE_READING_RAW			0x21

/**
 * CH_CMD_TAKE_READINGS:
 *
 * Take a reading taking into account just dark offsets.
 * All of @red, @green and @blue are _signed_ values.
 *
 * This command is useful if you want to do an ambient reading.
 *
 * IN:  [1:cmd]
 * OUT: [1:retval][1:cmd][2:red][2:green][2:blue]
 **/
#define	CH_CMD_TAKE_READINGS			0x22

/**
 * CH_CMD_TAKE_READING_XYZ:
 *
 * Take a reading taking into account:
 *  1. dark offsets
 *  2. the calibration matrix
 * All of @red, @green and @blue are _signed_ values
 * ranging from -1.0 to +1.0.
 *
 * IN:  [1:cmd][2:calibration-index]
 * OUT: [1:retval][1:cmd][2:red][2:green][2:blue]
 **/
#define	CH_CMD_TAKE_READING_XYZ			0x23

/**
 * CH_CMD_RESET:
 *
 * Reset the processor.
 *
 * IN:  [1:cmd]
 * OUT: [1:retval][1:cmd] (but with success the device will disconnect)
 **/
#define	CH_CMD_RESET				0x24

/**
 * CH_CMD_READ_FLASH:
 *
 * Read in raw data from the flash memory.
 *
 * IN:  [1:cmd][2:address][1:length]
 * OUT: [1:retval][1:cmd][1:checksum][1-60:data]
 **/
#define	CH_CMD_READ_FLASH			0x25

/**
 * CH_CMD_ERASE_FLASH:
 *
 * Erases flash memory before a write is done.
 * Erasing flash can only be done in 1k byte chunks and should be
 * aligned to 1k.
 *
 * IN:  [1:cmd][2:address][1:length]
 * OUT: [1:retval][1:cmd]
 **/
#define	CH_CMD_ERASE_FLASH			0x29

/**
 * CH_CMD_WRITE_FLASH:
 *
 * Write raw data to the flash memory. You can only write aligned to
 * a 32 byte boundary, and you must flush any incomplete 64 byte block.
 *
 * IN:  [1:cmd][2:address][1:length][1:checksum][1-32:data]
 * OUT: [1:retval][1:cmd]
 **/
#define	CH_CMD_WRITE_FLASH			0x26

/**
 * CH_CMD_BOOT_FLASH:
 *
 * Boot into to the flash memory.
 *
 * IN:  [1:cmd]
 * OUT: [1:retval][1:cmd]
 **/
#define	CH_CMD_BOOT_FLASH			0x27

/**
 * CH_CMD_SET_FLASH_SUCCESS:
 *
 * Sets the result of the firmware flashing. The idea of this command
 * is that the flashing interaction is thus:
 *
 * 1.	Reset()			device goes to bootloader mode
 * 2.	SetFlashSuccess(FALSE)
 * 3.	WriteFlash($data)
 * 4.	ReadFlash($data)	to verify
 * 5.	BootFlash()		switch to program mode
 * 6.	SetFlashSuccess(TRUE)
 *
 * The idea is that we only set the success FALSE from the bootoloader
 * to indicate that on booting we should not boot into the program.
 * We can only set the success true from the *new* program code so as
 * to verify that the new program boots, and can accept HID commands.
 *
 * IN:  [1:cmd][1:success]
 * OUT: [1:retval][1:cmd]
 **/
#define	CH_CMD_SET_FLASH_SUCCESS		0x28

/**
 * CH_CMD_GET_POST_SCALE:
 *
 * Get the post XYZ scaling constant.
 *
 * @scale is a value scaled between -8.0 and +8.0f
 * (so the divisor is 0x06)
 *
 * IN:  [1:cmd]
 * OUT: [1:retval][1:cmd][2:scale]
 **/
#define	CH_CMD_GET_POST_SCALE			0x2a

/**
 * CH_CMD_SET_POST_SCALE:
 *
 * Set the post XYZ scaling constant.
 *
 * @scale is a value scaled between 0.0 and 8.0f
 *
 * IN:  [1:cmd][2:scale]
 * OUT: [1:retval][1:cmd]
 **/
#define	CH_CMD_SET_POST_SCALE			0x2b

/* secret code */
#define	CH_WRITE_EEPROM_MAGIC			"Un1c0rn2"

/* input and output buffer offsets */
#define	CH_BUFFER_INPUT_CMD			0x00
#define	CH_BUFFER_INPUT_DATA			0x01
#define	CH_BUFFER_OUTPUT_RETVAL			0x00
#define	CH_BUFFER_OUTPUT_CMD			0x01
#define	CH_BUFFER_OUTPUT_DATA			0x02

/* where the custom firmware is stored */
#define CH_EEPROM_ADDR_RUNCODE			0x4000

/* although each calibration can be stored in 60 bytes,
 * we use a full 64 byte block */
#define	CH_CALIBRATION_MAX			64	/* so finishes at device params */

/* approximate sample times */
#define CH_INTEGRAL_TIME_VALUE_5MS		0x0300
#define CH_INTEGRAL_TIME_VALUE_50MS		0x1f00
#define CH_INTEGRAL_TIME_VALUE_100MS		0x3a00
#define CH_INTEGRAL_TIME_VALUE_200MS		0x7500
#define CH_INTEGRAL_TIME_VALUE_MAX		0xffff

/* flash constants */
#define	CH_FLASH_ERASE_BLOCK_SIZE		0x400	/* 1024 */
#define	CH_FLASH_WRITE_BLOCK_SIZE		0x040	/* 64 */
#define	CH_FLASH_TRANSFER_BLOCK_SIZE		0x020	/* 32 */

/* which color to select */
typedef enum {
	CH_COLOR_SELECT_RED,
	CH_COLOR_SELECT_WHITE,
	CH_COLOR_SELECT_BLUE,
	CH_COLOR_SELECT_GREEN
} ChColorSelect;

/* what frequency divider to use */
typedef enum {
	CH_FREQ_SCALE_0,
	CH_FREQ_SCALE_20,
	CH_FREQ_SCALE_2,
	CH_FREQ_SCALE_100
} ChFreqScale;

/* fatal error morse code */
typedef enum {
	CH_FATAL_ERROR_NONE,
	CH_FATAL_ERROR_UNKNOWN_CMD,
	CH_FATAL_ERROR_WRONG_UNLOCK_CODE,
	CH_FATAL_ERROR_NOT_IMPLEMENTED,
	CH_FATAL_ERROR_UNDERFLOW_SENSOR,
	CH_FATAL_ERROR_NO_SERIAL,
	CH_FATAL_ERROR_WATCHDOG,
	CH_FATAL_ERROR_INVALID_ADDRESS,
	CH_FATAL_ERROR_INVALID_LENGTH,
	CH_FATAL_ERROR_INVALID_CHECKSUM,
	CH_FATAL_ERROR_INVALID_VALUE,
	CH_FATAL_ERROR_UNKNOWN_CMD_FOR_BOOTLOADER,
	CH_FATAL_ERROR_NO_CALIBRATION,
	CH_FATAL_ERROR_OVERFLOW_MULTIPLY,
	CH_FATAL_ERROR_OVERFLOW_ADDITION,
	CH_FATAL_ERROR_OVERFLOW_SENSOR,
	CH_FATAL_ERROR_LAST
} ChFatalError;

#endif
