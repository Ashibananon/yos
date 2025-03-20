/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _Y_ADC_H_
#define _Y_ADC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum yadc_pin {
	YADC_PIN_INVALID = -1,
	YADC_PIN_0,
	YADC_PIN_1,
	YADC_PIN_2,
	YADC_PIN_3,
	/* 4, 5 are used by IIC */
	YADC_PIN_6,
	YADC_PIN_7
};

int yadc_init();
int yadc_deinit();

int yadc_begin(enum yadc_pin pin);
int yadc_read(uint16_t *value);
int yadc_end();


#ifdef __cplusplus
}
#endif
#endif
