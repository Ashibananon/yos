/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "yos/yos.h"
#include "yos/ytimer.h"
#include "ydevice/yadc.h"
#include "ydevice/yusart.h"
#include "ydevice/yiic.h"
#include "../lib/cmdline/basic_io.h"
#include "../lib/cmdline/cmdline.h"
#include "../lib/ssd1306xled/ssd1306xled/ssd1306xled.h"
#include "../lib/ssd1306xled/ssd1306xled/yos_ssd1306_font.h"
#include "../lib/ssd1306xled/ssd1306xled/ssd1306xledtx.h"
#include "../lib/AVR_aht20/src/aht20.h"
#include "ysm_28byj_48.h"

#define YIIC_MASTER_SPEED		100000

#define _WITH_TEMT6000_MODULE	1

static int8_t _temperature = 0;
static uint8_t _humidity = 0;

#if (_WITH_TEMT6000_MODULE == 1)
static uint16_t _light_value = 0;
#endif

#define _BASIC_IO_WRITE(msg)	basic_io_write(msg, strlen(msg), 1)

static void _ath20_event_cb(int8_t temperature, uint8_t humidity, enum AHT20_STATUS status)
{
	if (status == AHT20_SUCCESS) {
		_temperature = temperature;
		_humidity = humidity;
	}
}

static int _cmdline_task(void *para)
{
	while (1) {
		do_cmdline();
	}

	return 0;
}

static int _oled_task(void *para)
{
	ssd1306_init();
	ssd1306_clear();
	yos_ssd1306_puts(YOS_SSD1306_FONT_6X8, 0, 0, "YOS OLED");
	uint8_t dd, hh, mm, ss;
	dd = hh = mm = ss = 0;
	char buf[16];
	int invert = 0;

	while (1) {
		if (ss == 60) {
			ss = 0;
			mm++;
			if (mm == 60) {
				mm = 0;
				hh++;
				if (hh == 24) {
					hh = 0;
					dd++;
				}
			}
		}

		snprintf(buf, sizeof(buf), "%03u D", dd);
		yos_ssd1306_puts(YOS_SSD1306_FONT_6X8,
						yos_ssd1306_char_col_to_pixel(YOS_SSD1306_FONT_6X8, 0),
						2, buf);

		snprintf(buf, sizeof(buf), "%02u H", hh);
		yos_ssd1306_puts(YOS_SSD1306_FONT_6X8,
						yos_ssd1306_char_col_to_pixel(YOS_SSD1306_FONT_6X8, 6),
						2, buf);

		snprintf(buf, sizeof(buf), "%02u M", mm);
		yos_ssd1306_puts(YOS_SSD1306_FONT_6X8,
						yos_ssd1306_char_col_to_pixel(YOS_SSD1306_FONT_6X8, 11),
						2, buf);

		snprintf(buf, sizeof(buf), "%02u S", ss);
		yos_ssd1306_puts(YOS_SSD1306_FONT_6X8,
						yos_ssd1306_char_col_to_pixel(YOS_SSD1306_FONT_6X8, 16),
						2, buf);

		snprintf(buf, sizeof(buf), "T: %3d", _temperature);
		yos_ssd1306_puts(YOS_SSD1306_FONT_6X8,
						yos_ssd1306_char_col_to_pixel(YOS_SSD1306_FONT_6X8, 0),
						4, buf);

		snprintf(buf, sizeof(buf), "H: %3u%%", _humidity);
		yos_ssd1306_puts(YOS_SSD1306_FONT_6X8,
						yos_ssd1306_char_col_to_pixel(YOS_SSD1306_FONT_6X8, 8),
						4, buf);

#if (_WITH_TEMT6000_MODULE == 1)
		snprintf(buf, sizeof(buf), "Lt: %6u", _light_value);
		yos_ssd1306_puts(YOS_SSD1306_FONT_6X8,
						yos_ssd1306_char_col_to_pixel(YOS_SSD1306_FONT_6X8, 0),
						6, buf);
#endif

		if (ss == 0) {
			ssd1306_display_invert(invert);
			invert = !invert;
		}

		yos_task_msleep(1000);
		ss++;
	}

	return 0;
}

static int _aht20_task(void *para)
{
	aht20_init();
	register_aht20_event_callback(_ath20_event_cb);
	while (1) {
		aht20_trigger_measurement();
		yos_task_msleep(100);
		aht20_event();

		yos_task_msleep(1000);
	}

	return 0;
}

#if (_WITH_TEMT6000_MODULE == 1)
static int _temt6000_task(void *para)
{
	while (1) {
		if (yadc_begin(YADC_PIN_2) == 0) {
			if (yadc_read(&_light_value) != 0) {
				_light_value = 0xFFFF;
			}
		} else {
			_light_value = 0xFFFF;
		}
		yadc_end();

		yos_task_msleep(1000);
	}

	return 0;
}
#endif

#if 0
ISR(BADISR_vect)
{
	for (;;) UDR0='!';
}
#endif

int main()
{
	if (basic_io_init(yusart_io_operations) != 0) {
		return -1;
	}

#if 0
	/* Check reset flags */
	if(MCUSR & (1 << PORF )) _BASIC_IO_WRITE(PSTR("Power-on reset.\n"));
	if(MCUSR & (1 << EXTRF)) _BASIC_IO_WRITE(PSTR("External reset!\n"));
	if(MCUSR & (1 << BORF )) _BASIC_IO_WRITE(PSTR("Brownout reset!\n"));
	if(MCUSR & (1 << WDRF )) _BASIC_IO_WRITE(PSTR("Watchdog reset!\n"));
	MCUSR = 0;
#endif

	if (user_timer_init() != 0) {
		return -1;
	}

	if (yiic_master_init(YIIC_MASTER_SPEED) != 0) {
		return -1;
	}

#if (_WITH_TEMT6000_MODULE == 1)
	if (yadc_init() != 0) {
		return -1;
	}
#endif

	if (ysm_init() != 0) {
		return -1;
	}

	_BASIC_IO_WRITE("-------------\n");
	_BASIC_IO_WRITE("YOS starts\n");

	yos_init();

	if (yos_create_task(_cmdline_task, NULL, 250, "cmdtask") < 0) {
		return -1;
	}

	if (yos_create_task(_oled_task, NULL, 180, "oledtsk") < 0 ) {
		return -1;
	}

	if (yos_create_task(_aht20_task, NULL, 100, "ahttsk") < 0 ) {
		return -1;
	}

#if (_WITH_TEMT6000_MODULE == 1)
	if (yos_create_task(_temt6000_task, NULL, 100, "temttsk") < 0 ) {
		return -1;
	}
#endif

	yos_start();

	/*
	 * Not going here
	 *
	 * ここに着くことはありません
	 */
	return 0;
}
