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

/**
 * @brief ADC conversion complete callback.
 *
 * An ADC callback is called from ADC IRQ context when passed as an 
 * argument to adc_read() function.
 *
 * @param[in] channel channel number which was sampled during the conversion
 * @param[in] value value of ADC conversion
 * @return the function returns a channel number which will be sampled during next
           conversion cycle or -1 if no more conversions are required. If -1 is returned
		   then ADC module is disabled to reduce energy consumption.
 */
typedef int (*adc_cb_t)(int channel, uint16_t value);

/**
 * @brief ADC voltage reference source
 */
typedef enum {
	ADC_AREF = 0, /**< AREF pin, Internal Vref turned off */
	ADC_AVCC = 1, /**< AVCC with external capacitor at AREF pin */
	ADC_INT  = 3, /**< Internal 2.56V Voltage Reference with external capacitor at AREF pin */
} adc_ref_t;

/**
 * @brief ADC prescaler
 */
typedef enum {
	ADC_PRE2   = 1,
	ADC_PRE4   = 2,
	ADC_PRE8   = 3,
	ADC_PRE16  = 4,
	ADC_PRE32  = 5,
	ADC_PRE64  = 6,
	ADC_PRE128 = 7
} adc_prescaler_t;

typedef enum {
	ADC_NOISE_REDUCTION = 1,
	ADC_LEFT_ADJUST     = 2,
} adc_opts_t;

#ifndef ADC_BITS
#define ADC_BITS        1024          // 2^(ADC bits)
#endif

#ifndef ADV_VREF
#define ADV_VREF        2560          // in mV
#endif

#define ADC_MV(n)       ({uint32_t _r = ((uint32_t)n*(ADV_VREF))/(ADC_BITS); _r;})

void adc_init(adc_ref_t ref, adc_prescaler_t prescaler, int opts);
void adc_deinit(void);

uint16_t adc_read(int channel, adc_cb_t cb);
