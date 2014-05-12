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
#ifndef _MINIXIE_H_
#define _MINIXIE_H_

#include "logger.h"

#ifndef F_CPU
#define F_CPU 8000000UL
#endif

#ifndef LOG_SEVERITY
#define LOG_SEVERITY LOG_DEBUG
#endif

#ifndef UART_QUEUE_SIZE
#define UART_QUEUE_SIZE 50
#endif

#ifndef UART_BAUD_RATE
#define UART_BAUD_RATE  19200
#endif

// OC1A = PB1
// OC1B = PB2
// SMPS default params
#define SMPS_PWM_FREQ   50
#define SMPS_PWM_PERIOD (F_CPU/SMPS_PWM_FREQ/1000)
#define SMPS_PWM_DC     80 // in %
#define SMPS_ON()       do { TCCR1B |= _BV(CS10); TCCR1A |= _BV(COM1A1); } while (0) //no prescaling
#define SMPS_OFF()      do { TCCR1B &= ~(_BV(CS21) | _BV(CS11) | _BV(CS10)); TCCR1A &= ~_BV(COM1A1); } while (0)
#define SMPS_SET_DC(dc) (OCR1A = (dc*SMPS_PWM_PERIOD)/100)

// buttons pins
#define BTN_HH          (PIND & _BV(PD4))
#define BTN_MM          (PIND & _BV(PD3))

// ADC macros
#define ADC_HV          4
#define ADC_VL          5

#define HV_R6           268000UL        // in ohm
#define HV_R7           3240UL          // in ohm
#define HV_FROM_ADC(n)  ({uint32_t _r = ADC_MV(n)*(HV_R6+HV_R7)/HV_R7/1000; _r;})

#define DMUX_START()    (TCCR0 |= _BV(CS00))
#define DMUX_STOP()     (TCCR0 &= ~_BV(CS00))

#define PWM_TOP         50

// a structure to hold time
typedef struct {
	int hh;
	int mm;
	int ss;
} clock_t;

// a simple pad type definition
typedef struct {
	volatile uint8_t *port;
	int pin;
} pad_t;

// a convenience macro to define a pad
#define PAD(_port, _pin) {.port = _port, .pin = _pin}

// convenience macros to set a pad state
#define PAD_HIGH(_pad)   *((_pad)->port) |= (1 << (_pad)->pin)
#define PAD_LOW(_pad)    *((_pad)->port) &= ~(1 << (_pad)->pin)
#define PAD_TOGGLE(_pad) *((_pad)->port) ^= (1 << (_pad)->pin)

#define INLINE inline

#endif
