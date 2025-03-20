#include "i2c.h"
#include "../../../src/ydevice/yiic.h"

void iic_stub_init()
{
}

void iic_stub_start()
{
	yiic_master_trans_start();
}

void iic_stub_stop()
{
	yiic_master_trans_stop();
}

void iic_stub_write(uint8_t byte)
{
	yiic_master_transmit_data(&byte, sizeof(byte));
}

uint16_t iic_stub_read(void *buf, uint16_t buf_len)
{
	return yiic_master_receive_data(buf, buf_len);
}
