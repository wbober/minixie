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

#include <avr/sleep.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>
#include "adc.h"

#define ADC_SELECT_CHANNEL(pin)    (ADMUX = (ADMUX & 0xF0) | pin)
#define ADC_START_CONVERSION()     (ADCSRA |= _BV(ADSC))
#define ADC_ENABLE()               (ADCSRA |= _BV(ADEN))
#define ADC_DISABLE()              (ADCSRA &= ~_BV(ADEN))

static volatile struct 
{
	int noise_reduce;
	adc_cb_t cb;
	int channel;
} adc_ctx;

 // ADC conversion complete
ISR(ADC_vect)
{
	int ret = -1;
	uint16_t value = ADC;
	
	if (adc_ctx.cb != NULL)
		ret = adc_ctx.cb(adc_ctx.channel, value);

	if (ret >= 0) {
		adc_ctx.channel = ret;
		ADC_SELECT_CHANNEL(ret);
		ADC_START_CONVERSION();
	} else {
		ADC_DISABLE();
	}
}

/**
 * @brief Initialize ADC module.
 *
 */
void adc_init(adc_ref_t ref, adc_prescaler_t prescaler, int opts)
{
	// set conversion complete IRQ and prescaler
	ADCSRA = _BV(ADIE) | prescaler;
	// select ADC reference source
	ADMUX = prescaler << 6;

	if (opts & _BV(ADC_NOISE_REDUCTION))
		adc_ctx.noise_reduce |= _BV(ADC_NOISE_REDUCTION);

	if (opts & _BV(ADC_LEFT_ADJUST))
		ADMUX |= _BV(ADLAR);
}

/**
 * @brief Deinitialize ADC module.
 */
inline void adc_deinit(void)
{
	ADC_DISABLE();
}

/**
 * @brief Read ADC value
 *
 * This function allows for poll or interrupt based ADC reading. If a pointer
 * to a callback function is given then adc_read() finishes immediately. The
 * callback function will be called *FROM THE INTTERUPT CONTEXT* once the
 * conversion is complete. 
 
 * If cb is NULL then adc_read() waits until the conversion is complete.
 *
 * @param[in] channel ADC channel which should be sampled
 * @param[in] cb a pointer to callback funcion which should be called
                 once the conversion is complete.
 */
uint16_t adc_read(int channel, adc_cb_t cb)
{
	uint16_t value = UINT16_MAX;
	
	// if ADC is enabled then wait for the current 
	// conversion to complete (if there is one in progress)
	if (ADCSRA & _BV(ADEN)) {
		while (ADCSRA & _BV(ADSC));
	} else {
		ADC_ENABLE();
	}

	ADC_SELECT_CHANNEL(channel);

	if (cb != NULL) {
		adc_ctx.cb = cb;
		if (adc_ctx.noise_reduce) {
			set_sleep_mode(SLEEP_MODE_ADC);
			sleep_mode();
		} else {
			ADC_START_CONVERSION();
		}
	} else {
		ADC_START_CONVERSION();
		while (ADCSRA & _BV(ADSC))
			;
		ADC_DISABLE();
		value = ADC;
	}

	return value;
}
