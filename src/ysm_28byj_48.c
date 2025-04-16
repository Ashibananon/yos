/*
 * Step Motor 28BYJ-48 Driver
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <avr/io.h>
#include "yos/yos.h"
#include "ysm_28byj_48.h"

#define _SM_INT_PORT_DIR	DDRD
#define _SM_INT1_PORT_DIR	DDD7
#define _SM_INT2_PORT_DIR	DDD6
#define _SM_INT3_PORT_DIR	DDD5
#define _SM_INT4_PORT_DIR	DDD4

#define _SM_INT_PORT		PORTD
#define _SM_INT1			PD7
#define _SM_INT2			PD6
#define _SM_INT3			PD5
#define _SM_INT4			PD4

static void _ysm_set_int_value(int sm_int, int value)
{
	if (sm_int == _SM_INT1 || sm_int == _SM_INT2 || sm_int == _SM_INT3 || sm_int == _SM_INT4) {
		if (value) {
			_SM_INT_PORT |= (1 << sm_int);
		} else {
			_SM_INT_PORT &= (~(1 << sm_int));
		}
	}
}

static void _ysm_int_reset()
{
	_ysm_set_int_value(_SM_INT1, 0);
	_ysm_set_int_value(_SM_INT2, 0);
	_ysm_set_int_value(_SM_INT3, 0);
	_ysm_set_int_value(_SM_INT4, 0);
}

int ysm_init()
{
	_SM_INT_PORT_DIR |= ((1 << _SM_INT1_PORT_DIR) | (1 << _SM_INT2_PORT_DIR)
							| (1 << _SM_INT3_PORT_DIR) | (1 << _SM_INT4_PORT_DIR));
	_ysm_int_reset();

	return 0;
}

int ysm_deinit()
{
	_ysm_int_reset();
	_SM_INT_PORT_DIR &= (~((1 << _SM_INT1_PORT_DIR) | (1 << _SM_INT2_PORT_DIR)
							| (1 << _SM_INT3_PORT_DIR) | (1 << _SM_INT4_PORT_DIR)));
	return 0;
}


static uint8_t _ysm_pulse_values[] = {
	0x01,
	0x03,
	0x02,
	0x06,
	0x04,
	0x0C,
	0x08,
	0x09
};

static void _ysm_rotate_pulse(enum ysm_direction dir)
{
	uint8_t start;
	uint8_t cnt;
	int8_t delta;
	if (dir == YSM_DIRECTION_CLOCKWISE) {
		start = 0;
		delta = 1;
	} else {
		start = sizeof(_ysm_pulse_values) / sizeof(_ysm_pulse_values[0]) - 1;
		delta = -1;
	}

	cnt = 0;
	while (cnt < sizeof(_ysm_pulse_values) / sizeof(_ysm_pulse_values[0])) {
		_ysm_set_int_value(_SM_INT1, _ysm_pulse_values[start] & 0x01);
		_ysm_set_int_value(_SM_INT2, _ysm_pulse_values[start] & 0x02);
		_ysm_set_int_value(_SM_INT3, _ysm_pulse_values[start] & 0x04);
		_ysm_set_int_value(_SM_INT4, _ysm_pulse_values[start] & 0x08);
		yos_task_msleep(10);

		start += delta;
		cnt++;
	}
}

int ysm_rotate(enum ysm_direction dir, uint16_t pulse_sets)
{
	uint16_t pulse_set_cnt = 0;

	while (pulse_set_cnt < pulse_sets) {
		_ysm_rotate_pulse(dir);

		pulse_set_cnt++;
	}

	_ysm_int_reset();

	return 0;
}
