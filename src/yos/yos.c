/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

 #include <avr/interrupt.h>
#include <avr/io.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "yos_core.h"
#include "ystack.h"
#include "ymutex.h"

static struct yos_task _all_tasks[YOS_MAX_TASK_COUNT];
static void *volatile stack_bp_for_next_task;
static volatile int task_id_for_next;

static volatile int _CURRENT_TASK_ID;
static struct yos_task *volatile _CURRENT_TASK = NULL;

static volatile uint8_t _tmp_sp_high_byte;
static volatile uint8_t _tmp_sp_low_byte;

/*
 * Task shell function, the parameter [task_id] must be passed
 * by stack.
 *
 * It seems that the avr-gcc tends to pass parameters by
 * regs(or memory) rather than stack.
 * See: https://gcc.gnu.org/wiki/avr-gcc#Calling_Convention
 *
 * However, as far as I know, va_args seems to be passed by stack.
 *
 *
 * タスクシェル関数です、パラメーター「task_id」はスタック通じて
 * 渡されないといけません。
 *
 * avr-gccのドキュメントによって、関数の呼び出す時、パラメーターは
 * スタックではなく、レジスタを通じて渡されます。
 * 参照：https://gcc.gnu.org/wiki/avr-gcc#Calling_Convention
 *
 * だがわたしの知り限り、va_argsは例外で、いつもスタックでパラメーター
 * を渡されるようでしょう。
 */
static void _yos_task_shell(int task_id, ...)
{
	YOS_DBG("enter yos task shell(id=%d)\n", task_id);
	if (task_id < 0 || task_id >= YOS_MAX_TASK_COUNT) {
		goto resched;
	}
	struct yos_task *task = _all_tasks + task_id;
	int task_ret = -1;
	if (task->task_func != NULL) {
		task->status = YOS_TASK_STATUS_RUNNING;
		task_ret = task->task_func(task->data);
	}
	task->status = YOS_TASK_STATUS_EXITED;

resched:
	schedule();
}

static void _yos_init_task_stack(int task_id, struct yos_task *task)
{
	uint8_t *sp = (uint8_t *)(task->bp);

	/* Parameter [task_id] */
	*(sp--) = U16_HIGH_BYTE(task_id);
	*(sp--) = U16_LOW_BYTE(task_id);

	/*
	 * TODO:
	 * I don't know why the following 2 bytes are needed.
	 *
	 * TODO:
	 * 以下の2バイトは必要ですが、原因はまだ分かりません。
	 */
	*(sp--) = 0;
	*(sp--) = 0;

	/*
	 * Return address is set with [&_yos_task_shell], so that when it
	 * returns from this stack, [_yos_task_shell] will be called with
	 * parameter [task_id] which is located 3-4 bytes above.
	 *
	 * 戻りアドレスは「&_yos_task_shell」として設定してから、このスタック
	 * から戻ると、「_yos_task_shell」が呼び出されます、パラメーター
	 * 「task_id」は先程3-4バイト上のところに設定されています
	 */
	*(sp--) = U16_LOW_BYTE(&_yos_task_shell);
	*(sp--) = U16_HIGH_BYTE(&_yos_task_shell);

	/* __temp_reg__ */
	*(sp--) = 0;

	task->sp = sp;
}

/*
 * Find the next runnable task one by one.
 *
 * 順番に次の実行できるタスクを探します
 */
static int _find_next_task_to_run()
{
	struct yos_task *_the_next_task;
	int the_next_task_id = (_CURRENT_TASK_ID + 1) % YOS_MAX_TASK_COUNT;
	while (the_next_task_id != _CURRENT_TASK_ID) {
		_the_next_task = _all_tasks + the_next_task_id;
		if (_the_next_task->status == YOS_TASK_STATUS_CREATED
			|| _the_next_task->status == YOS_TASK_STATUS_RUNNING
			|| _the_next_task->status == YOS_TASK_STATUS_EXITED) {
			/*
			 * Found!
			 *
			 * 見つけた！
			 */
			break;
		}

		the_next_task_id = (the_next_task_id + 1) % YOS_MAX_TASK_COUNT;
	}

	return the_next_task_id;
}

static void _update_task_block_ticks_irq()
{
	struct yos_task *_the_task;
	int i = 0;
	while (i < YOS_MAX_TASK_COUNT) {
		_the_task = _all_tasks + i;
		if (_the_task->status == YOS_TASK_STATUS_WAITING) {
			if (_the_task->block_ticks > 0) {
				_the_task->block_ticks--;
			}
			if (_the_task->block_ticks == 0) {
				_the_task->status = YOS_TASK_STATUS_RUNNING;
			}
		}

		i++;
	}
}

static void _global_timer_interrupt_enable(int enable)
{
	if (enable) {
		TIMSK0 |= (1 << TOIE0);
	} else {
		TIMSK0 &= (~(1 << TOIE0));
	}
}

static void _global_timer_start()
{
	/* 1ms */
	TCNT0 = 6;

	_global_timer_interrupt_enable(1);
}

static void _global_timer_stop()
{
	_global_timer_interrupt_enable(0);
}

static void _global_timer_init()
{
	TCCR0A = 0;
	TCCR0B = ((1 << CS01) | (1 << CS00));

	_global_timer_start();
}

static void _global_timer_deinit()
{
	TCCR0B = 0;
}


static volatile uint8_t _task_switch_counter = 0;
static volatile int need_to_switch_context;
static volatile int need_to_restore_reg;
static volatile int next_task_id;
static struct yos_task *volatile next_task;
static volatile int8_t _is_scheduled_by_int = 1;
static volatile int8_t _need_schedule = 0;
ISR(TIMER0_OVF_vect, __attribute__((naked)))
{
	SAVE_REGS_TO_STACK

	if (_is_scheduled_by_int) {
		TCNT0 = 6;
		_task_switch_counter++;
		if (_task_switch_counter == _TASK_SWITCH_INTERVAL_MS) {
			_task_switch_counter = 0;
			_update_task_block_ticks_irq();
			_need_schedule = 1;
		}
	} else {
		_need_schedule = 1;
		_is_scheduled_by_int = 1;
	}

	need_to_switch_context = 0;
	need_to_restore_reg = 1;
	next_task = NULL;
	if (_need_schedule) {
		_need_schedule = 0;

		/*
		 * schedule tasks
		 *
		 * タスクをスケジュールします
		 */
		next_task_id = _find_next_task_to_run();
		if (next_task_id == _CURRENT_TASK_ID) {
		} else {
			next_task = _all_tasks + next_task_id;
			if (next_task->status == YOS_TASK_STATUS_CREATED) {
				_yos_init_task_stack(next_task_id, next_task);

				need_to_switch_context = 1;
				need_to_restore_reg = 0;
			} else if (next_task->status == YOS_TASK_STATUS_RUNNING) {
				need_to_switch_context = 1;
			} else if (next_task->status == YOS_TASK_STATUS_EXITED) {
				next_task->status = YOS_TASK_STATUS_INVALID;
			} else {
				/*
				 * Invalid task
				 *
				 * 不可能なタスク
				 */
			}
		}
	}

	if (need_to_switch_context) {
		__asm__ __volatile__ (
			/*
			 * Save current SP
			 *
			 * 「今」動いているタスクのSPを保存します
			 */
			"push __tmp_reg__					\n\t"
			"in __tmp_reg__, __SP_L__			\n\t"
			"sts _tmp_sp_low_byte, __tmp_reg__	\n\t"
			"in __tmp_reg__, __SP_H__			\n\t"
			"sts _tmp_sp_high_byte, __tmp_reg__	\n\t"
		);
		_CURRENT_TASK->sp = (void *)(U8HL_TO_U16(_tmp_sp_high_byte, _tmp_sp_low_byte));

#if (YOS_RECORD_STACK_USAGE == 1)
		if ((uint16_t)(_CURRENT_TASK->min_sp_by_now) > (uint16_t)(_CURRENT_TASK->sp)) {
			_CURRENT_TASK->min_sp_by_now = _CURRENT_TASK->sp;
		}
#endif

		_tmp_sp_high_byte = U16_HIGH_BYTE(next_task->sp);
		_tmp_sp_low_byte = U16_LOW_BYTE(next_task->sp);
		__asm__ __volatile__ (
			/*
			 * Restore next SP
			 *
			 * 次の動くタスクのSPを回復します
			 */
			"lds __tmp_reg__, _tmp_sp_low_byte	\n\t"
			"out __SP_L__, __tmp_reg__			\n\t"
			"lds __tmp_reg__, _tmp_sp_high_byte	\n\t"
			"out __SP_H__, __tmp_reg__			\n\t"
			"pop __tmp_reg__					\n\t"
		);

		_CURRENT_TASK = next_task;
		_CURRENT_TASK_ID = next_task_id;
		_CURRENT_TASK->status = YOS_TASK_STATUS_RUNNING;
	}

	if (need_to_restore_reg) {
		RESTORE_REGS_FROM_STACK
	}

	reti();
}

int yos_create_task(int (*task_func)(void *task_data), void *data,
						uint16_t stack_size, char *name)
{
	int task_id;
	if (task_func == NULL || stack_size == 0 || task_id_for_next >= YOS_MAX_TASK_COUNT) {
		return -1;
	}

	task_id = task_id_for_next;
	struct yos_task *this_task = _all_tasks + task_id;
	this_task->task_func = task_func;
	this_task->data = data;
	this_task->bp = stack_bp_for_next_task;
	this_task->sp = this_task->bp;
#if (YOS_RECORD_STACK_USAGE == 1)
	this_task->min_sp_by_now = this_task->sp;
#endif
	this_task->stack_size = stack_size;
	this_task->status = YOS_TASK_STATUS_CREATED;
	this_task->block_ticks = 0;
	if (name != NULL) {
		strncpy(this_task->name, name, sizeof(this_task->name));
	} else {
		snprintf(this_task->name, sizeof(this_task->name), "Tsk%03d", task_id);
	}
	this_task->name[sizeof(this_task->name) - 1] = '\0';

	YOS_DBG("create task[%s], id=%d, bp=0x%04X, ss=%d\n",
			this_task->name, task_id, this_task->bp, this_task->stack_size);

	task_id_for_next++;
	stack_bp_for_next_task -= stack_size;

	return task_id;
}

int yos_delete_task(int task_id)
{
	/*
	 * TO delete a task is not supported right now, since it is not worthy
	 * to implement a memory management function at a 2KB-only-RAM system
	 *
	 * タスクの削除は利用できません。
	 * 2KBのSRAMしか搭載されないMCUなので、メモリー管理機能を実現することは
	 * 贅沢過ぎますでしょう。
	 */
	return -1;
}

#if (YOS_DEBUG_MSG_OUTPUT == 1)
#define YOS_IDLE_TASK_STACK_SIZE		256
#else
#define YOS_IDLE_TASK_STACK_SIZE		50
#endif

#define YOS_IDLE_TASK_NAME				"yosidle"
/*
 * YOS Idle task
 * Nothing has to be done right now
 *
 * The idle task makes sure that there is always a runnable task.
 *
 * アイドルタスクです。
 * 何もしないままになります
 *
 * アイドルタスクということは、いつでも動けるタスクが存在することを保証します。
 */
static int _yos_idle_task(void *para)
{
	int16_t i = 0;
	YOS_DBG("_yos_idle_task is running\n");
	while (1) {
		i++;
		//schedule();
	}

	return 0;
}

void yos_init()
{
	stack_bp_for_next_task = (void *)RAMEND;
	task_id_for_next = 0;

	int i = 0;
	while (i < YOS_MAX_TASK_COUNT) {
		_all_tasks[i].status = YOS_TASK_STATUS_INVALID;

		i++;
	}

	_CURRENT_TASK_ID = yos_create_task(_yos_idle_task,
										NULL,
										YOS_IDLE_TASK_STACK_SIZE,
										YOS_IDLE_TASK_NAME);

	YOS_DBG("yos_create_task returned %d\n", _CURRENT_TASK_ID);
}

void yos_start()
{
	int first_task_id = _find_next_task_to_run();
	YOS_DBG("enter yos_start, tid=%d\n", first_task_id);
	if (first_task_id < 0 || first_task_id >= YOS_MAX_TASK_COUNT) {
		return;
	}

	cli();

	_global_timer_init();

	_CURRENT_TASK_ID = first_task_id;
	_CURRENT_TASK = _all_tasks + first_task_id;
	_yos_init_task_stack(_CURRENT_TASK_ID, _CURRENT_TASK);

	_tmp_sp_high_byte = U16_HIGH_BYTE(_CURRENT_TASK->sp);
	_tmp_sp_low_byte = U16_LOW_BYTE(_CURRENT_TASK->sp);
	__asm__ __volatile__ (
		"lds __tmp_reg__, _tmp_sp_low_byte			\n\t"
		"out __SP_L__, __tmp_reg__					\n\t"
		"lds __tmp_reg__, _tmp_sp_high_byte			\n\t"
		"out __SP_H__, __tmp_reg__					\n\t"
		"pop __tmp_reg__							\n\t"
		"sei										\n\t"
		"ret"
	);

	/*
	 * This function will not return to its caller(e.g. main) but
	 * to the _yos_task_shell with first_task_id as parameter.
	 * All the stacks by the end of this point will be gone.
	 *
	 * この関数は呼び出し元に戻りません。
	 * 代わりに、「_yos_task_shell」に戻るようになります。
	 * それに、ここまでのスタックは全部なくなります。
	 */
}

static void _schedule_irq()
{
	_is_scheduled_by_int = 0;
	__vector_16();
}

void yos_task_delay(uint16_t ticks)
{
	cli();
	_CURRENT_TASK->status = YOS_TASK_STATUS_WAITING;
	_CURRENT_TASK->block_ticks = ticks;
	_schedule_irq();
	sei();
}

void yos_task_msleep(uint16_t ms)
{
	yos_task_delay(ms < _TASK_SWITCH_INTERVAL_MS ? 1 : ms / _TASK_SWITCH_INTERVAL_MS);
}

#define DEBUG_SCHEDULE_WITH_DELAY	0
void schedule()
{
#if (DEBUG_SCHEDULE_WITH_DELAY == 1)
	yos_task_delay(0);
#else
	cli();
	_schedule_irq();
	sei();
#endif
}


int yos_get_task_info(int task_id, struct yos_task_info *task_info)
{
	cli();

	int ret = -1;
	if (task_id < 0 || task_id >= YOS_MAX_TASK_COUNT) {
		goto para_err;
	}

	struct yos_task *this_task = _all_tasks + task_id;
	if (this_task->status == YOS_TASK_STATUS_CREATED
		|| this_task->status == YOS_TASK_STATUS_RUNNING
		|| this_task->status == YOS_TASK_STATUS_WAITING
		|| this_task->status == YOS_TASK_STATUS_EXITED) {
		if (task_info != NULL) {
			task_info->id = task_id;
			task_info->status = this_task->status;
			task_info->stack_size = this_task->stack_size;
#if (YOS_RECORD_STACK_USAGE == 1)
			task_info->stack_max_reached_size =
					(uint16_t)(this_task->bp) - (uint16_t)(this_task->min_sp_by_now);
#else
			task_info->stack_max_reached_size = 0;
#endif
			strcpy(task_info->name, this_task->name);
		}

		ret = 0;
	} else {
		ret = -1;
	}

para_err:
	sei();

	return ret;
}


#define _YMUTEX_OWNER_NONE		-1

void ymutex_init(struct ymutex *mutex)
{
	if (mutex == NULL) {
		return;
	}

	mutex->owner = _YMUTEX_OWNER_NONE;
}

void ymutex_lock(struct ymutex *mutex)
{
	if (mutex == NULL) {
		return;
	}

	while (1) {
		if (ymutex_try_lock(mutex) == 0) {
			break;
		}
		schedule();
	}
}

int ymutex_try_lock(struct ymutex *mutex)
{
	int ret = -1;
	if (mutex == NULL) {
		return ret;
	}

	cli();
	if (mutex->owner < 0) {
		mutex->owner = _CURRENT_TASK_ID;
		ret = 0;
	}
	sei();

	return ret;
}

int ymutex_unlock(struct ymutex *mutex)
{
	int ret = -1;
	if (mutex == NULL) {
		return ret;
	}

	cli();
	if (mutex->owner == _CURRENT_TASK_ID) {
		mutex->owner = _YMUTEX_OWNER_NONE;
		ret = 0;
	}
	sei();

	return ret;
}
