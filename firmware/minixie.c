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
#include <stdlib.h>
#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include "logger.h"
#include "dcf77.h"
#include "minixie.h"
#include "uart.h"
#include "adc.h"

uint16_t timer = 0;

/* 
 * Anode mapping table:
 * HH -> PB5
 * HL -> PB4
 * MH -> PD5
 * ML -> PD6
 * DOT -> PB3
 */
static const pad_t const anode_pad_map[] = {
	PAD(&PORTB, 5),
	PAD(&PORTB, 4),
	PAD(&PORTD, 5),
	PAD(&PORTD, 6),
	PAD(&PORTB, 3),
};

static const pad_t const digit_pad_map[] = {
	PAD(&PORTC, 0),
	PAD(&PORTC, 2),
	PAD(&PORTC, 3),
	PAD(&PORTC, 1),
};

static const pad_t buzzer_pad = PAD(&PORTB, 2);

static const uint16_t const dc_light_map[][2] = {
	{700,  80},
	{800,  70},
	{900,  60},
	{1000, 50},
};

typedef void (*pt)(void);

// digit mapping table
static const uint8_t const dt[] = {0,9,8,1,4,7,2,3,6,5};

/**
 * Module context - holds variables related to the module state.
 *
 */
typedef struct  {
	int tick: 1;
	int poll: 1;
	int uart: 1;
	int beep: 1;
	int dot : 1;

	int debug: 1;
	int input: 1;

	int dcf_sync: 1;
	int dcf_irq: 1;
	int dcf_debug: 1;
	int dcf_sync_cnt;

	uint8_t duty_cycle;
	uint8_t dot_pwm;

	uint8_t digit[4];

	uint16_t adc_hv;
	uint16_t adc_light;

	clock_t time;
	clock_t alarm;

} ctx_t;

volatile static ctx_t ctx = {
	.tick = 1,
	.dot = 1,
	.debug = 0,
	.beep = 0,
	.dcf_sync_cnt = 0,
	.duty_cycle = SMPS_PWM_DC,
	.dot_pwm = 0,
	.digit = {0},
	.adc_hv = 0,
	.adc_light = 0,
	.alarm = {255},
};

static void digit_mux(void);
static void rtc_tick(void);
static void dc_toggle(void);

// External interrupt
ISR(INT0_vect)
{
	if (dcf77_handler() > 0) {
		ctx.dcf_sync_cnt++;
		ctx.dcf_sync = 1;
	}
	ctx.dcf_irq = 1;
}

// Mux timer
ISR(TIMER0_OVF_vect)
{
	digit_mux();
}

// SPMS PWM timer
//ISR(TIMER1_COMPA_vect)
//{
//}

// RTC clock timer - IRQ invoked once a second
ISR(TIMER2_OVF_vect)
{
	timer += 256;
	rtc_tick();
}

// Analog comparator
ISR(ANA_COMP_vect)
{
	dc_toggle();
}

static void hw_init(void)
{
	// set the anode pins as an output
	DDRB = _BV(PB2) | _BV(PB3) | _BV(PB4) | _BV(PB5);
	DDRD = _BV(PD5) | _BV(PD6);
	// set the digit pins as an output
	DDRC = _BV(PC0) | _BV(PC1) | _BV(PC2) | _BV(PC3);
	
	// set btn pins as pulled-up an input
	//PORTD = _BV(PD2) | _BV(PD3) | _BV(PD4);
	PORTD = _BV(PD4);
	PORTB = _BV(PB0);

	// BTN1 as output (low)
	DDRD |= _BV(PD3); // for dcf power ctrl
	
	// Enable IRQ0 on PD2 
	MCUCR |= _BV(ISC00);
	GICR |= _BV(INT0);


	// Tubes' mux timer
	// generate an IRQ on overflow
	TIMSK |= _BV(TOIE0);

	// Fast PWM with ICR1 as TOP
	// with OC1A as output pin (non-inverting)
	// (note: for inverting add _BV(COM1A0))
	TCCR1A = _BV(WGM11) | _BV(COM1A1);
	TCCR1B = _BV(WGM13) | _BV(WGM12);
	DDRB |= _BV(PB1);

	ICR1 = SMPS_PWM_PERIOD;
	OCR1A = (ctx.duty_cycle*SMPS_PWM_PERIOD)/100;

	// RTC timer
	// clock div 128, normal mode
	TCCR2 = _BV(CS22) | _BV(CS20);
	// enable asynchronous operation
	ASSR = _BV(AS2);
	// generate an IRQ on timer overflow
	TIMSK |= _BV(TOIE2);

	// Analog Comparator, rising edge
	ACSR = _BV(ACBG) | _BV(ACIE) | _BV(ACIS1) | _BV(ACIS0);
}

// Tubes' mux handler - invoked at F_CPU/256 ~= 31.250 khz
static inline
void digit_mux(void)
{
	static unsigned pwm_ocr = 0;
	static unsigned pwm_cnt = 0;
	static unsigned mux_cnt = 0;
	static const pad_t *active_anode = anode_pad_map;

	if (pwm_cnt + 1 < pwm_ocr) {
		PAD_HIGH(active_anode);
	} else {
		PAD_LOW(active_anode);
	}

	// this gives 625Hz PWM frequency 
	if (++pwm_cnt == PWM_TOP) {
		pwm_cnt = 0;
	}
	
	// this gives 250Hz anode mulitplexing
	if (mux_cnt++ < 128) {
		return;
	}
	mux_cnt = 0;

	PAD_LOW(active_anode);
	
	if (active_anode < anode_pad_map + 4)
		active_anode += 1;
	else
		active_anode = anode_pad_map;
	
	if (active_anode < anode_pad_map + 4) {
		int digit = ctx.digit[active_anode - anode_pad_map];
		for (int i = 0; i < 4; i++) {
			if (dt[digit] & _BV(i))
				PAD_HIGH(&digit_pad_map[i]);
			else
				PAD_LOW(&digit_pad_map[i]);
		}
		pwm_ocr = PWM_TOP;
	} else {
		if (ctx.dot) {
			ctx.dot_pwm += (ctx.dot_pwm < PWM_TOP*2/3) ? 1 : 0;
		} else  {
			ctx.dot_pwm -= (ctx.dot_pwm > 0) ? 1 : 0;
		}
		pwm_ocr = ctx.dot_pwm;
	}
}

// RTC tick handler
static inline
void rtc_tick(void)
{
	ctx.poll = 1;
	ctx.tick = 1;
	ctx.dot ^= 1;
	
	if (ctx.time.ss < 59) {
		ctx.time.ss++;
		return;
	}
	ctx.time.ss = 0;

	if (ctx.time.mm < 59) {
		ctx.time.mm++;
		return;
	}
	ctx.time.mm = 0;

	if (ctx.time.hh < 23) {
		ctx.time.hh++;
		return;
	}
	ctx.time.hh = 0;
}

/**
 * Function called when ADC conversion is done.
 *
 */
int adc_cb(int channel, uint16_t value)
{
	if (channel == ADC_HV) {
		ctx.adc_hv = value;
		return ADC_VL;
	} else {
		ctx.adc_light = value;
		return ADC_HV;
	}
}

/**
 * Initializes power saving mode from IRQ.
 *
 */
static void dc_toggle(void)
{
	SMPS_OFF();
	DMUX_STOP();
}

/**
 * Character received callback.
 *
 */
void uart_rx_cb(uint8_t id)
{
	ctx.uart = 1;
}

/**
 * Parse a command.
 */ 
static void parse_command(void)
{
	static char buffer[15] = {0};
	static unsigned i = 0;
	char *bp;

	ctx.input = 1;
	
	while (i < sizeof(buffer) - 1) {
		if (!uart_read(0, (uint8_t *)buffer + i, 1))
			break;
		uart_write(0, (uint8_t *)buffer + i, 1);
		i++;
	}
	ctx.uart = 0;

	if (buffer[i - 1] == '\r' || buffer[i - 1] == '\n') {
		if ((bp = strstr(buffer, "smps off"))) {
			SMPS_OFF();
			DMUX_STOP();
		} else if ((bp = strstr(buffer, "smps on"))) {
			SMPS_ON();
			DMUX_START();
		} else if ((bp = strstr(buffer, "smps dc"))) {
			ctx.duty_cycle = atoi(bp + 8);
			SMPS_SET_DC(ctx.duty_cycle);
		} else if (strstr(buffer, "beep")) {
			ctx.beep = 1;
		} else if (strstr(buffer, "reset")) {
			while (1);
		} else if ((bp = strstr(buffer, "set"))) {
			bp[6] = 0;	
			bp[9] = 0;
			bp[12] = 0;
			ctx.time.hh = atoi(bp + 4);
			ctx.time.mm = atoi(bp + 7);
			ctx.time.ss = atoi(bp + 10);
		} else if ((bp = strstr(buffer, "alarm"))) {
			bp[8] = 0;	
			bp[11] = 0;
			bp[14] = 0;
			ctx.alarm.hh = atoi(bp + 6);
			ctx.alarm.mm = atoi(bp + 9);
			ctx.alarm.ss = atoi(bp + 12);
			log_debug("Alarm %02d:%02d:%02d", ctx.alarm.hh, ctx.alarm.mm, ctx.alarm.ss);
		} else if ((bp = strstr(buffer, "dbg on"))) {
			if (strstr(bp, "dcf")) 
				ctx.dcf_debug = 1;
			else
				ctx.debug =1;
		} else if ((bp = strstr(buffer, "dbg off"))) {
			if (strstr(bp, "dcf")) 
				ctx.dcf_debug = 0;
			else
				ctx.debug = 0;
		} else if ((bp = strstr(buffer, "dcf"))) {
			log_info("Sync cnt %d, last sync %02d/%02d/%02d %02d:%02d:%02d", \
					 ctx.dcf_sync_cnt, 
					 dcf_time.day, dcf_time.month, dcf_time.year, 
					 dcf_time.hour, dcf_time.minute);
		}

		i = 0;
		ctx.input = 0;
	} else if (i == sizeof(buffer) - 1) {
		for (unsigned j = i; j > 0; j--) {
			buffer[j - 1] = buffer[j];
		}
		i--;
	}

	buffer[i] = 0;
}

static void check_buttons(void)
{
	if (BTN_HH == 0) {
		// debounce
		_delay_ms(20);
		if (BTN_HH == 0) {
			ctx.time.hh = (ctx.time.hh < 23) ? ctx.time.hh + 1 : 0;
		}
	}
	if (BTN_MM == 0) {
		// debounce
		_delay_ms(20);
		if (BTN_MM == 0) {
			ctx.time.mm = (ctx.time.mm < 59) ? ctx.time.mm + 1 : 0;
		}
	}
}

/**
 * Update display.
 * 
 * This function calculates new digit values and updates
 * SMPS DC according to light conditions.
 *
 */
static void refresh(void)
{
	ctx.digit[0] = ctx.time.hh / 10;
	ctx.digit[1] = ctx.time.hh % 10;
	ctx.digit[2] = ctx.time.mm / 10;
	ctx.digit[3] = ctx.time.mm % 10;

	ctx.adc_light = adc_read(ADC_VL, NULL);

	#if 0
	ctx.duty_cycle = SMPS_PWM_DC;
	for (int i = 0; i < sizeof(dc_light_map)/sizeof(dc_light_map[0]); i++) {
		if (ctx.adc_light > dc_light_map[i][0])
			ctx.duty_cycle = dc_light_map[i][1];
	}
	SMPS_SET_DC(ctx.duty_cycle);
	#endif
}

/**
 * Main loop when MCU is active.
 *
 */
__attribute__((OS_main))
int main(void)
{
	hw_init();

	uart_init(UART0, UART_BAUD_SELECT(UART_BAUD_RATE), uart_rx_cb, NULL);
	log_init();

	sei();
	
	log_info("Init done!");

	while (1) {
		adc_init(ADC_INT, ADC_PRE128, 0);
		
		SMPS_ON();
		DMUX_START();

		wdt_enable(WDTO_2S);
		set_sleep_mode(SLEEP_MODE_IDLE);

		while (bit_is_clear(ACSR, ACO)) {
			if (ctx.tick) {
				ctx.tick = 0;
				refresh();
				if (ctx.debug && !ctx.input) {
					ctx.adc_hv = adc_read(ADC_HV, NULL);
					uint32_t hv = HV_FROM_ADC(ctx.adc_hv);
					log_debug("HV:%ld Light:%d DC:%d", hv, ctx.adc_light, ctx.duty_cycle);
					log_debug("Local time: %02d:%02d:%02d", ctx.time.hh, ctx.time.mm, ctx.time.ss);
				}
			}

			if (ctx.uart) {
				parse_command();
			}

			if (ctx.beep) {
				ctx.beep = 0;
				PAD_HIGH(&buzzer_pad);
				_delay_ms(20);
				PAD_LOW(&buzzer_pad);
			}

			if (ctx.poll) {
				ctx.poll = 0;
				check_buttons();
			}

			if (ctx.dcf_sync) {
				ctx.dcf_sync = 0;
				ctx.time.hh = dcf_time.hour;
				ctx.time.mm = dcf_time.minute;
				ctx.time.ss = 0;
			}

			if (ctx.dcf_irq && ctx.dcf_debug && !ctx.input) {
				ctx.dcf_irq = 0;
				log_debug("DCF state: %d", dcf_state);
			}

			sleep_mode();
			wdt_reset();
		}

		wdt_disable();

		adc_deinit();
		uart_deinit(UART0);

		PORTB &= 0xE0;
		PORTC &= 0xF0;
		PORTD &= 0x9C;

		set_sleep_mode(SLEEP_MODE_PWR_SAVE);

		while (bit_is_set(ACSR, ACO)) {
			ACSR |= _BV(ACD);
			_delay_us(50);
			sleep_mode();
			ACSR &= ~_BV(ACD);
		}
		
		uart_init(UART0, UART_BAUD_SELECT(UART_BAUD_RATE), uart_rx_cb, NULL);
	}
}
