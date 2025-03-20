/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _YIIC_H_
#define _YIIC_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * IIC PINs
 * SCL -- PC5
 * SDA -- PC4
 */


enum yiic_master_tr {
	YIIC_MASTER_TRANSMIT = 0,
	YIIC_MASTER_RECEIVE = 1
};

int yiic_master_init(uint32_t clk_freq);
int yiic_master_deinit();

void yiic_master_wait();
int yiic_master_trans_start();
int yiic_master_trans_stop();
int yiic_master_select_slave(uint8_t addr, enum yiic_master_tr t_or_r);
uint16_t yiic_master_transmit_data(void *data, uint16_t data_len);
uint16_t yiic_master_receive_data(void *buff, uint16_t buf_len);


#ifdef __cplusplus
}
#endif
#endif
