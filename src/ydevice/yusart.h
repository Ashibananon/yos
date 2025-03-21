/*
 * YOS
 *
 * Copyright(C) 2025 Ashibananon(Yuan).
 *
 */

#ifndef _YOS_USART_H_
#define _YOS_USART_H_

#include <stdint.h>
#include "common_def.h"
#include "../../lib/cmdline/basic_io.h"

#ifdef __cplusplus
extern "C" {
#endif

#define USART_BAUDRATE		250000
#define USART_STOPBITS		1

#define YUSART_RECEIVE_BUFFER_SIZE_BYTE			64

extern struct basic_io_port_operations *yusart_io_operations;

#ifdef __cplusplus
}
#endif
#endif
