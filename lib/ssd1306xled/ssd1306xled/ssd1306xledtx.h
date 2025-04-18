/**
 * SSD1306xLED - Library for the SSD1306 based OLED/PLED 128x64 displays
 * @author Neven Boyanov
 * This is part of the Tinusaur/SSD1306xLED project.
 * ----------------------------------------------------------------------------
 *  Copyright (c) 2023 Tinusaur (https://tinusaur.com). All rights reserved.
 *  Distributed as open source under the MIT License (see the LICENSE.txt file)
 *  Please, retain in your work a link to the Tinusaur project website.
 * ----------------------------------------------------------------------------
 * Source code available at: https://gitlab.com/tinusaur/ssd1306xled
 */

#ifndef SSD1306XLEDTX_H
#define SSD1306XLEDTX_H

// ============================================================================

#define USE_YOS_TEXT_FONT		1

#if (WITH_TX_NUM_SUPPORT == 1)
#define ssd1306_numdec(n) ssd1306tx_numdec(n)
#endif

// ----------------------------------------------------------------------------

#if (USE_YOS_TEXT_FONT == 1)
#include "yos_ssd1306_font.h"
#else
extern uint8_t *ssd1306xled_font6x8;
extern uint8_t *ssd1306xled_font8x16;
#endif

// ----------------------------------------------------------------------------
#if (USE_YOS_TEXT_FONT == 1)
#else
void ssd1306tx_init(const uint8_t *font_src, uint8_t char_base);

void ssd1306tx_char(char ch);
void ssd1306tx_string(char *s);
#endif

#if (WITH_TX_NUM_SUPPORT == 1)
void ssd1306tx_numdec(uint16_t num);
void ssd1306tx_numdecp(uint16_t num);
#endif

#if (USE_YOS_TEXT_FONT == 1)
#else
void ssd1306tx_stringxy(const uint8_t *font_src, uint8_t x, uint8_t y, const char s[]);
#endif
// ============================================================================

#endif
