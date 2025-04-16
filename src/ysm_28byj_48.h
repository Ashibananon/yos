/*
 * Step Motor 28BYJ-48 Driver
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _YSM_28BYJ_48_H_
#define _YSM_28BYJ_48_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum ysm_direction {
	YSM_DIRECTION_CLOCKWISE,
	YSM_DIRECTION_COUNTER_CLOCKWISE
};

/*
 * To rotate one circle (2 * PI), the number of pulse-sets must be performed
 * at a direction.
 *
 * モーターを一周（2 * PI　弧度）で回転させるため、一定の数のパルスセットをモーターに
 * 行わせることが必要です。
 */
#define YSM_PULSE_SETS_TO_STEP_ONE_CIRCLE		(64 * 64 / 8)

/*
 * Init Step Motor
 *
 * モータードライバー初期化
 *
 * Return
 *		0		Init OK
 *		Others	Init Error
 *
 * 返却値
 *		0		ドライバー初期化成功
 *		その他  ドライバー初期化失敗
 */
int ysm_init();

/*
 * Deinit Step Motor
 *
 * モータードライバー解放
 *
 * Return
 *		0		Deinit OK
 *		Others	Deinit Error
 *
 * 返却値
 *		0		ドライバー解放成功
 *		その他  ドライバー解放失敗
 */
int ysm_deinit();

/*
 * Rotate with specified direction and num of pulse sets
 *
 * 方向とパルスセット数を指定してモーターを回転させる
 *
 * Return
 *		0		Rotate OK
 *		Others	Rotate Error
 *
 * 返却値
 *		0		回転成功
 *		その他  回転失敗
 */
int ysm_rotate(enum ysm_direction dir, uint16_t pulse_sets);

#ifdef __cplusplus
}
#endif
#endif
