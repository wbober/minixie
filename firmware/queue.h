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
#ifndef _QUEUE_H_
#define _QUEUE_H_

typedef struct {
	uint8_t front, rear;
	uint8_t size;
	uint8_t length;
	uint8_t *data;
} queue_t;

#define Q_INIT(_SIZE)\
    (&((queue_t){\
        .length = 0, \
        .size = _SIZE,\
        .data = (uint8_t[_SIZE]){},\
     }))

static inline int q_put(queue_t *q, uint8_t b)
{
	uint8_t rear = (q->rear + 1) % q->size;
	if (rear == q->front) {
		return 0;
	}
	q->rear = rear;
	q->data[q->rear] = b;
	q->length += 1;
	return 1;
}

static inline int q_get(queue_t *q, uint8_t *result)
{
	if (q->front == q->rear) {
		return 0;
	}
	q->front = (q->front + 1) % q->size;
	q->length -= 1;
	*result = q->data[q->front];
	return 1;
}

#define q_is_empty(q) (q->length == 0)

#endif
