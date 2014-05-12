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

#include <string.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include "uart.h"

static inline void uart_tx(uint8_t u_id);
static inline void uart_rx(uint8_t u_id);

static uart_ctx_t uart_ctx[] =
{
	{
		.u_id = 0,
		.u_mode = USART_ACTIVE,
		.pUDR   = (uint8_t *)&UDR,
		.pUCSRA = (uint8_t *)&UCSRA,
		.pUCSRB = (uint8_t *)&UCSRB,
		.pUCSRC = (uint8_t *)&UCSRC,
		.pUBRRL = (uint8_t *)&UBRRL,
		.pUBRRH = (uint8_t *)&UBRRH,
		.rx_queue = Q_INIT(UART_QUEUE_SIZE),
		.tx_queue = Q_INIT(UART_QUEUE_SIZE)
	},
};

ISR(USART_RXC_vect)
{
	uart_rx(UART0);
}

ISR(USART_UDRE_vect)
{
	uart_tx(UART0);
}

/**
 \brief USART TX interrupt handler
 \param usart_id USART port number
 */
static inline void uart_tx(uint8_t u_id)
{
	psart_ctx_t u = (psart_ctx_t) &uart_ctx[u_id];
	if (!q_is_empty(u->tx_queue)) {
		uint8_t data = 0;
		q_get(u->tx_queue, &data);
		*u->pUDR = data;
	} else {
		*u->pUCSRB &= ~_BV(UDRIE);
	}

	if (u->tx_cb != NULL) {
		u->tx_cb(u_id);
	}
}

/**
 \brief USART RX interrupt handler
 \param usart_id USART port number
 */
static inline void uart_rx(uint8_t u_id)
{
	psart_ctx_t u = (psart_ctx_t) &uart_ctx[u_id];
	uint8_t c = UDR;
	q_put(u->rx_queue, c);

	if (u->rx_cb != NULL) {
		u->rx_cb(u_id);
	}
}

/**
 \brief Set USART mode.
 */
uint8_t uart_mode(psart_ctx_t u, uint8_t mode)
{
	if (u->u_mode == mode)
		return 0;

	switch (mode) {
		case USART_ACTIVE:
			*u->pUCSRB |= _BV(RXCIE);
			break;

		case UART_INACTIVE:
			*u->pUCSRB &= ~_BV(RXCIE);
			break;
	}

	u->u_mode = mode;
	return 1;
}

/**
 \brief Read number of bytes from given usart's circular buffer

 The invoker is responsible for providing sufficient space in the
 buffer. If requested number of bytes is larger then number of bytes
 present in the buffer function fails.

 \param u usart context
 \param bp destination buffer
 \param n number of bytes to read
 */
uint8_t uart_read(uint8_t u_id, uint8_t *bp, uint8_t n)
{
	psart_ctx_t u = (psart_ctx_t) &uart_ctx[u_id];
	
	cli();
	uint8_t i;
	
	for (i = 0;i < n; i++) {
		if (!q_get(u->rx_queue, bp++)) {
			break;
		}
	}
	sei();
	
	return i;
}

/**
 \brief Write number of bytes in given usart's context circular buffer.

 Function will fail if free space in the buffer is smaller then number
 of bytes requested to be written.

 \param u usart context
 \param bp pointer to source buffer
 \param n number of bytes to write from source buffer
 */
uint8_t uart_write(uint8_t u_id, uint8_t *bp, uint8_t n)
{
	psart_ctx_t u = (psart_ctx_t) &uart_ctx[u_id];

	cli();
	uint8_t i;
	
	for (i = 0; i < n; i++) {
		if (!q_put(u->tx_queue, *bp++))
			break;
	}
	sei();

	if (!(*u->pUCSRB & _BV(UDRIE))) {
		*u->pUCSRB |= _BV(UDRIE);
	}

	return i;
}

/**
 Initialise UART
 */
void uart_init(uint8_t u_id, uint16_t baud, uart_cb_t rx_cb, uart_cb_t tx_cb)
{
	psart_ctx_t u = &uart_ctx[u_id];
	u->rx_cb = rx_cb;
	u->tx_cb = tx_cb;
	*u->pUBRRL = baud & 0xFF;
	*u->pUBRRH = baud >> 8;
	*u->pUCSRB = _BV(RXCIE) | _BV(RXEN) | _BV(TXEN);
	*u->pUCSRC = _BV(URSEL) | _BV(UCSZ1) | _BV(UCSZ0);
}

void uart_deinit(uint8_t u_id)
{
	psart_ctx_t u = &uart_ctx[u_id];
	*u->pUCSRB &= ~(_BV(RXEN) | _BV(TXEN | _BV(RXCIE) | _BV(UDRIE)));
}

