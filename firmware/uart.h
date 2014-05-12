/**
 * Minixie - a simple nixie tube clock.
 * Copyright (C) 2012-2014, Wojciech Bober
 *
 * License:
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
#ifndef _USART_H_
#define _USART_H_

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "queue.h"

#ifndef UART_QUEUE_SIZE
#define UART_QUEUE_SIZE    100
#endif

#define UART_WAIT_COUNT    100

#define USART_OK           1
#define USART_ERROR        0

#define UART0              0
#define UART1              1

typedef enum {
    UART_INACTIVE,
    USART_ACTIVE,
} usart_mode_t;

typedef void (*uart_cb_t)(uint8_t u_id);

typedef struct {
    uint8_t u_id;
    usart_mode_t u_mode;
    queue_t* rx_queue;
    queue_t* tx_queue;

    uart_cb_t rx_cb;
    uart_cb_t tx_cb;

    uint8_t *pUDR;
    uint8_t *pUCSRA;
    uint8_t *pUCSRB;
    uint8_t *pUCSRC;
    uint8_t *pUBRRL;
    uint8_t *pUBRRH;
} uart_ctx_t, *psart_ctx_t;

#define UART_BAUD_SELECT(baudRate) ((F_CPU)/16/(baudRate)-1)

void uart_init(uint8_t u_id, uint16_t baud, uart_cb_t rx_cb, uart_cb_t tx_cb);
void uart_deinit(uint8_t u_id);

uint8_t uart_write(uint8_t u_id, uint8_t *bp, uint8_t n);
uint8_t uart_read(uint8_t u_id, uint8_t *bp, uint8_t n);

#endif
