/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "yusart.h"
#include "../../lib/ringbuf/yringbuffer.h"

static char _yusart_receive_buffer[YUSART_RECEIVE_BUFFER_SIZE_BYTE];
static struct YRingBuffer _yusart_rx_rb;

static void yusart_interrupt_disable()
{
	UCSR0B &= (uint8_t)(~((1 << RXCIE0) | (1 << TXCIE0) | (1 << UDRIE0)));
}

static void yusart_interrupt_enable()
{
	UCSR0B |= (uint8_t)((1 << RXCIE0) | (1 << TXCIE0) | (1 << UDRIE0));
}

static void yusart_init(uint32_t baudrate, uint8_t stop_bits)
{
	cli();

	uint32_t ubrr = (FOSC / 16 / baudrate - 1);
	uint8_t sb = ((stop_bits == 2) ? (1 << USBS0) : 0);

	YRingBufferInit(&_yusart_rx_rb, _yusart_receive_buffer, sizeof(_yusart_receive_buffer));

	/*Set baud rate */
	UBRR0H = (unsigned char)(ubrr >> 8);
	UBRR0L = (unsigned char)ubrr;
	/* Enable receiver and transmitter */
	UCSR0B = (1 << RXEN0) | (1 << TXEN0);
	/* Set frame format: 8data, 2stop bit */
	UCSR0C = sb | (3 << UCSZ00);
	yusart_interrupt_enable();

	sei();
}

/* Interrupt: RX Complete */
ISR(USART_RX_vect)
{
	char d = UDR0;
	YRingBufferPutData(&_yusart_rx_rb, &d, sizeof(d), 1);
}

/* Interrupt: USART Data Register Empty(For Transmit) */
ISR(USART_UDRE_vect)
{
}

/* Interrupt: TX Complete */
ISR(USART_TX_vect)
{
}

static void yusart_deinit()
{
	cli();

	yusart_interrupt_disable();
	UCSR0B = 0;

	YRingBufferDestory(&_yusart_rx_rb);

	sei();
}

static int yusart_can_transmit()
{
	int ret;

	cli();
	yusart_interrupt_disable();
	ret = (UCSR0A & ( 1 << UDRE0));
	yusart_interrupt_enable();
	sei();

	return ret;
}

static int yusart_can_receive()
{
	int can;

	cli();
	yusart_interrupt_disable();
	can = YRingBufferGetSize(&_yusart_rx_rb) > 0;
	yusart_interrupt_enable();
	sei();

	return can;
}

static int yusart_io_init()
{
	yusart_init(USART_BAUDRATE, USART_STOPBITS);

	return 0;
}

static int yusart_io_deinit()
{
	yusart_deinit();

	return 0;
}

static int yusart_io_read_byte_no_block(uint8_t *b)
{
	int ret = -1;
	uint8_t data;

	cli();
	yusart_interrupt_disable();
	if (YRingBufferGetData(&_yusart_rx_rb, &data, sizeof(data)) == sizeof(data)) {
		if (b != NULL) {
			*b = data;
		}
		ret = 0;
	}
	yusart_interrupt_enable();
	sei();

	return ret;
}

static int yusart_io_write_byte_no_block(uint8_t b)
{
	int ret = -1;

	cli();
	yusart_interrupt_disable();
	if (UCSR0A & ( 1 << UDRE0)) {
		UDR0 = b;
		ret = 0;
	}
	yusart_interrupt_enable();
	sei();

	return ret;
}


static struct basic_io_port_operations _yusart_io_operations = {
	.basic_io_port_init = yusart_io_init,
	.basic_io_port_deinit = yusart_io_deinit,
	.basic_io_port_can_read = yusart_can_receive,
	.basic_io_port_read_byte_no_block = yusart_io_read_byte_no_block,
	.basic_io_port_can_write = yusart_can_transmit,
	.basic_io_port_write_byte_no_block = yusart_io_write_byte_no_block
};

struct basic_io_port_operations *yusart_io_operations = &_yusart_io_operations;
