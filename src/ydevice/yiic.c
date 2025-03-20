/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <stdio.h>
#include "avr/io.h"
#include "util/twi.h"
#include "../common_def.h"
#include "yiic.h"
#include "../yos/ymutex.h"

static struct ymutex _yiic_mutex;

int yiic_master_init(uint32_t clk_freq)
{
	if (clk_freq == 0) {
		return -1;
	}

	uint32_t f_min = (FOSC / (16 + 2 * (255 * 64)));
	uint32_t f_max = (FOSC / 16);
	if (clk_freq < f_min || clk_freq > f_max) {
		return -1;
	}

	uint32_t twbr_psca = (FOSC / clk_freq - 16) / 2;
	uint8_t twps10;
	uint16_t twbr;
	if (twbr_psca <= 255) {
		twps10 = 0;
		twbr = (twbr_psca / 1);
	} else if (twbr_psca > 255 && twbr_psca <= 1020) {
		twps10 = (1 << TWPS0);
		twbr = (twbr_psca / 4);
	} else if (twbr_psca > 1020 && twbr_psca <= 4080) {
		twps10 = (1 << TWPS1);
		twbr = (twbr_psca / 16);
	} else if (twbr_psca > 4080 && twbr_psca <= 16320) {
		twps10 = ((1 << TWPS1) | (1 << TWPS0));
		twbr = (twbr_psca / 64);
	} else {
		return -1;
	}

	TWSR |= twps10;
	TWBR = (twbr & 0xFF);

	TWCR = (1 << TWEN);

	ymutex_init(&_yiic_mutex);

	return 0;
}

int yiic_master_deinit()
{
	TWSR = 0;
	TWBR = 0;
	TWCR = 0;

	return 0;
}

void yiic_master_wait()
{
	while (!(TWCR & (1 << TWINT))) {
	}
}

int yiic_master_trans_start()
{
	ymutex_lock(&_yiic_mutex);

	int ret;
	TWCR = ((1 << TWEN) | (1 << TWINT) | (1 << TWSTA));
	yiic_master_wait();
	ret = (((TWSR & TW_STATUS_MASK) == TW_START) ? 0 : -1);
	//TWCR &= (~(1 << TWSTA));

	return ret;
}

int yiic_master_trans_stop()
{
	TWCR = ((1 << TWEN) | (1 << TWINT) | (1 << TWSTO));
	while (TWCR & (1 << TWSTO)) {
	}

	ymutex_unlock(&_yiic_mutex);

	return 0;
}

int yiic_master_select_slave(uint8_t addr, enum yiic_master_tr t_or_r)
{
	TWDR = (addr << 1 | (t_or_r == YIIC_MASTER_TRANSMIT ? 0 : 1));
	TWCR = ((1 << TWEN) | (1 << TWINT));
	yiic_master_wait();

	return ((TWSR & TW_STATUS_MASK) ==
				((t_or_r == YIIC_MASTER_TRANSMIT) ? TW_MT_SLA_ACK : TW_MR_SLA_ACK))
			? 0 : -1;
}

uint16_t yiic_master_transmit_data(void *data, uint16_t data_len)
{
	uint16_t bytes_transmited = 0;
	if (data == NULL) {
		goto para_err;
	}

	while (bytes_transmited < data_len) {
		TWDR = *((uint8_t *)data + bytes_transmited);
		TWCR = ((1 << TWEN) | (1 << TWINT));
		yiic_master_wait();
		if ((TWSR & TW_STATUS_MASK) != TW_MT_DATA_ACK) {
			/* Error */
			break;
		}

		bytes_transmited++;
	}

para_err:
	return bytes_transmited;
}

uint16_t yiic_master_receive_data(void *buff, uint16_t buf_len)
{
	uint16_t bytes_received = 0;
	if (buff == NULL) {
		goto para_err;
	}

	int is_last_byte;
	while (bytes_received < buf_len) {
		is_last_byte = (bytes_received == buf_len - 1);

		if (is_last_byte) {
			/* Last byte */
			TWCR = ((1 << TWEN) | (1 << TWINT));
		} else {
			/* Not last byte */
			TWCR |= ((1 << TWEN) | (1 << TWINT) | (1 << TWEA));
		}

		yiic_master_wait();
		*((uint8_t *)buff + bytes_received) = TWDR;

		if ((TWSR & TW_STATUS_MASK) != (is_last_byte ? TW_MR_DATA_NACK : TW_MR_DATA_ACK)) {
			/* Error */
			break;
		}

		bytes_received++;
	}

para_err:
	return bytes_received;
}
