#ifndef __I2C_H_
#define __I2C_H_

#include <avr/io.h>

#define I2C_F 100000UL

#define I2C_READ	1
#define I2C_WRITE	0

#define I2C_ACK		1
#define I2C_NACK	0

void iic_stub_init();
void iic_stub_start();
void iic_stub_stop();
void iic_stub_write(uint8_t byte);
uint16_t iic_stub_read(void *buf, uint16_t buf_len);

#endif
