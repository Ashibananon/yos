/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>
#include "ytimer.h"

static struct user_timer {
	void (*on_timeout)(void *para);
	void *para;
	uint32_t remaining_ms;
	uint32_t init_ms;
	uint8_t auto_restart;
	uint8_t is_paused;
	uint8_t is_used;
} _user_timer_list[USER_TIMER_MAX_COUNT];

static void _user_timer_list_check_in_irq()
{
	int i = 0;
	struct user_timer *ut;
	while (i < USER_TIMER_MAX_COUNT) {
		ut = _user_timer_list + i;
		if (ut->is_used) {
			if (!(ut->is_paused)) {
				if (ut->remaining_ms == 0) {
					if (ut->on_timeout != NULL) {
						ut->on_timeout(ut->para);
					}

					if (ut->auto_restart) {
						ut->remaining_ms = ut->init_ms;
					} else {
						ut->is_used = 0;
					}
				} else {
					ut->remaining_ms--;
				}
			}
		}

		i++;
	}
}

static void _user_timer_interrupt_enable(int enable)
{
	if (enable) {
		TIMSK2 |= (1 << TOIE2);
	} else {
		TIMSK2 &= (~(1 << TOIE2));
	}
}

static void _user_timer_list_init()
{
	memset(&_user_timer_list, 0x00, sizeof(_user_timer_list));
}

int user_timer_init()
{
	_user_timer_list_init();

	TCCR2A = 0;
	TCCR2B = (1 << CS22);

	/* 1ms */
	TCNT2 = 6;
	/* Enable interrupt */
	_user_timer_interrupt_enable(1);

	return 0;
}

int user_timer_deinit()
{
	_user_timer_interrupt_enable(0);
	TCCR2B = 0;

	return 0;
}

ISR(TIMER2_OVF_vect)
{
	/* 1ms */
	TCNT2 = 6;

	_user_timer_list_check_in_irq();
}

int user_timer_create(uint32_t timeout_ms, int auto_restart,
	void (*on_timeout)(void *para), void *timeout_para)
{
	int timer_id = -1;
	int i = 0;

	struct user_timer *ut;
	_user_timer_interrupt_enable(0);
	while (i < USER_TIMER_MAX_COUNT) {
		ut = _user_timer_list + i;
		if (!(ut->is_used)) {
			ut->on_timeout = on_timeout;
			ut->para = timeout_para;
			ut->remaining_ms = timeout_ms;
			ut->init_ms = timeout_ms;
			ut->auto_restart = auto_restart;
			ut->is_paused = 0;
			ut->is_used = 1;

			timer_id = i;
			break;
		}

		i++;
	}
	_user_timer_interrupt_enable(1);

	return timer_id;
}

static int _user_timer_pause_set(int timer_id, uint8_t pause)
{
	if (timer_id < 0 || timer_id > USER_TIMER_MAX_COUNT) {
		return -1;
	}

	struct user_timer *ut;
	_user_timer_interrupt_enable(0);
	ut = _user_timer_list + timer_id;
	if (ut->is_used) {
		ut->is_paused = pause;
	}
	_user_timer_interrupt_enable(1);

	return 0;
}

int user_timer_pause(int timer_id)
{
	return _user_timer_pause_set(timer_id, 1);
}

int user_timer_restore(int timer_id)
{
	return _user_timer_pause_set(timer_id, 0);
}

int user_timer_reset(int timer_id, uint32_t timeout_ms)
{
	if (timer_id < 0 || timer_id > USER_TIMER_MAX_COUNT) {
		return -1;
	}

	struct user_timer *ut;
	_user_timer_interrupt_enable(0);
	ut = _user_timer_list + timer_id;
	if (ut->is_used) {
		ut->remaining_ms = timeout_ms;
		ut->init_ms = timeout_ms;
	}
	_user_timer_interrupt_enable(1);

	return 0;
}

uint32_t user_timer_get_remaining_ms(int timer_id)
{
	if (timer_id < 0 || timer_id > USER_TIMER_MAX_COUNT) {
		return 0;
	}

	uint32_t remaining = 0;
	struct user_timer *ut;
	_user_timer_interrupt_enable(0);
	ut = _user_timer_list + timer_id;
	if (ut->is_used) {
		remaining = ut->remaining_ms;
	}
	_user_timer_interrupt_enable(1);

	return remaining;
}

int user_timer_destroy(int timer_id)
{
	if (timer_id < 0 || timer_id > USER_TIMER_MAX_COUNT) {
		return -1;
	}

	struct user_timer *ut;
	_user_timer_interrupt_enable(0);
	ut = _user_timer_list + timer_id;
	if (ut->is_used) {
		//ut->on_timeout = NULL;
		//ut->para = NULL;
		//ut->remaining_ms = 0;
		//ut->auto_restart = 0;
		//ut->is_paused = 0;
		ut->is_used = 0;
	}
	_user_timer_interrupt_enable(1);

	return 0;
}
