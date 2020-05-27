/***************************************************************************//**
 * @file
 * @brief Configuration file for textdisplay module.
 *******************************************************************************
 * # License
 * <b>Copyright 2018 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 ******************************************************************************/

#ifndef TEXTDISPLAYCONFIG_H
#define TEXTDISPLAYCONFIG_H

/* Include display configuration files here because the textdisplay
   configuration depends on the display configuration. */
#ifdef HAL_CONFIG
#include "displayhalconfig.h"
#else
#include "displayconfig.h"
#endif
#include "displayconfigapp.h"

/**
 * Maximum number of text display devices the display module is configured
 * to support. This number may be increased if the system includes more than
 * one display device. However, the number should be kept low in order to
 * save memory.
 */
#define TEXTDISPLAY_DEVICES_MAX   (1)

/* Font definitions depending on which font is selected. */
#ifdef TEXTDISPLAY_FONT_8x8
  #define FONT_WIDTH   (8)
  #define FONT_HEIGHT  (8)
#endif
#ifdef TEXTDISPLAY_FONT_6x8
  #define FONT_WIDTH   (6)
  #define FONT_HEIGHT  (8)
#endif
#ifdef TEXTDISPLAY_NUMBER_FONT_16x20
  #define FONT_WIDTH   (16)
  #define FONT_HEIGHT  (20)
#endif

/**
 * Determine the number of lines and columns of the text display devices.
 * These constants are used for static memory allocation in the textdisplay
 * device driver.
 *
 * Please make sure that the combined selection of font, lines and columns fits
 * inside the DISPLAY geometry.
 */
#ifndef TEXTDISPLAY_DEVICE_0_LINES
#define TEXTDISPLAY_DEVICE_0_LINES        (DISPLAY0_HEIGHT / FONT_HEIGHT)
#endif
#define TEXTDISPLAY_DEVICE_0_COLUMNS      (DISPLAY0_WIDTH / FONT_WIDTH)

/* Enable PixelMatrix allocation support in the display device driver.
   The textdisplay module allocates a pixel matrix corresponding to one line of
   text on the display. Therefore we need support for pixel matrix allocation.
 */
#define PIXEL_MATRIX_ALLOC_SUPPORT

/* Enable allocation of pixel matrices from the static pixel matrix pool.
   NOTE:
   The allocator does not support free'ing pixel matrices. It allocates
   continuosly from the static pool without keeping track of the sizes of
   old allocations. I.e. this is a one-shot allocator, and the  user should
   allocate buffers once at the beginning of the program.
 */
#define USE_STATIC_PIXEL_MATRIX_POOL

/* Specify the size of the static pixel matrix pool. For the textdisplay
   we need one line of text, that is, the font height (8) times the
   display width (128 pixels divided by 8 bits per byte). */
#ifndef PIXEL_MATRIX_POOL_SIZE
#define PIXEL_MATRIX_POOL_SIZE       (FONT_HEIGHT * DISPLAY0_WIDTH / 8)
#endif
/* The alignment of the pixel matrices must depend on the font width
   in order to be handled correctly.*/
#define PIXEL_MATRIX_ALIGNMENT  (FONT_WIDTH / 8 + ((FONT_WIDTH % 8) ? 1 : 0))

#endif /* TEXTDISPLAYCONFIG_H */
