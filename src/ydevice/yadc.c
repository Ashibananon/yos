/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <stdio.h>
#include <avr/io.h>
#include "yadc.h"
#include "../yos/ymutex.h"


static struct ymutex _yadc_mutex;
static volatile enum yadc_pin _selected_pin = YADC_PIN_INVALID;

int yadc_init()
{
	ADCSRA = (1 << ADEN);
	ADCSRB = 0;
	ymutex_init(&_yadc_mutex);

	return 0;
}

int yadc_deinit()
{
	ADCSRA = 0;

	return 0;
}


int yadc_begin(enum yadc_pin pin)
{
	ymutex_lock(&_yadc_mutex);

	int ret = 0;
	switch (pin) {
	case YADC_PIN_0:
		ADMUX = 0;
		DIDR0 |= (1 << ADC0D);
		break;
	case YADC_PIN_1:
		ADMUX = 1;
		DIDR0 |= (1 << ADC1D);
		break;
	case YADC_PIN_2:
		ADMUX = 2;
		DIDR0 |= (1 << ADC2D);
		break;
	case YADC_PIN_3:
		ADMUX = 3;
		DIDR0 |= (1 << ADC3D);
		break;
	case YADC_PIN_6:
		ADMUX = 6;
		break;
	case YADC_PIN_7:
		ADMUX = 7;
		break;
	default:
		ret = -1;
		break;
	}

	if (ret == 0) {
		_selected_pin = pin;
	} else {
		_selected_pin = YADC_PIN_INVALID;
	}

	return ret;
}

int yadc_read(uint16_t *value)
{
	int ret = 0;
	if (_selected_pin != YADC_PIN_INVALID) {
		ADCSRA |= (1 << ADSC);
		while (ADCSRA & (1 << ADSC)) {
			/* Wait until adc done */
		}
		if (value != NULL) {
			/* ADCL must be read first */
			//*value = (ADCL | (uint16_t)(ADCH << 8));
			*value = ADC;
		}
	} else {
		ret = -1;
	}

	return ret;
}

int yadc_end()
{
	int ret = 0;
	if (_selected_pin != YADC_PIN_INVALID) {
		switch (_selected_pin) {
		case YADC_PIN_0:
			ADMUX = 15;
			DIDR0 &= (~(1 << ADC0D));
			break;
		case YADC_PIN_1:
			ADMUX = 15;
			DIDR0 &= (~(1 << ADC1D));
			break;
		case YADC_PIN_2:
			ADMUX = 15;
			DIDR0 &= (~(1 << ADC2D));
			break;
		case YADC_PIN_3:
			ADMUX = 15;
			DIDR0 &= (~(1 << ADC3D));
			break;
		case YADC_PIN_6:
			ADMUX = 15;
			break;
		case YADC_PIN_7:
			ADMUX = 15;
			break;
		default:
			ret = -1;
			break;
		}
	} else {
		ret = -1;
	}

	_selected_pin = YADC_PIN_INVALID;

	ymutex_unlock(&_yadc_mutex);

	return ret;
}
