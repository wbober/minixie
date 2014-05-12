/**********************************************************************
 * AVR DCF77 decoder (v0.1) by Rickard Gunée, 2007                
 *
 * About this program:
 * This is a decoder of the German 77.5KHz time code "DCF77".
 * The decoder is quite flexible in its design and can quite easy be
 * modified for your own purposes. It is built basically by one state
 * machine and a simple command parser, the state machine handles the
 * timing of the pulse width moduleated bits including the sync bit.
 * Then the command parser decodes the bits into a meaningful data
 * structure, each command has information on how many bits to decode
 * and if parity shall be counted and most important, how it shall be
 * decoded. If you only want part of the timecode then you can change
 * the struct and some of the decoding commands to ignore commands to
 * be able to get smaller memory footprint. This program was intended
 * to run on an AVR MCU but could easily be fitted for any MCU.
 * The decoder is intended to be polled at the frequency defined in
 * the dcf77.h file but you can quite easily change it to be interrupt
 * driven if you have a hardware timer that can replace the
 * "period_timer"-variable.
 *                 
 * For more info:                                          
 * see www.rickard.gunee.com/projects            
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
 */
 

// frequency in which the the dcf handler is polled
#define DCF_HANDLER_FREQ	256


// pin and port used for connection to the DCF-receiver
#define DCF_PORT	        PIND
#define DCF_BIT		        PIND2

// dcf bit reading macro, modify this for example if
// your signal is inverted
#define dcf_pin (DCF_PORT & (1<<DCF_BIT))

// timing constants based on the handler update frequency
// you probably don't need to change these unless you have
// some kind of LP-filter on the input or somehting similar
#define DCF_TIME_L				(DCF_HANDLER_FREQ*3/20)
#define DCF_TIME_H_MAX			(DCF_HANDLER_FREQ*3/10)
#define DCF_TIME_L_MAX			(DCF_HANDLER_FREQ*19/20)
#define DCF_TIME_L_MIN			(DCF_HANDLER_FREQ*1/20)
#define DCF_TIME_SYNC_MIN		(DCF_HANDLER_FREQ*3/2)

// states for the dcf bit timing state machine
#define DCF_S_ERROR				0x00

#define DCF_S_WAIT				0x00
#define DCF_S_SYNC				0x01
#define DCF_S_DATA_L			0x02
#define DCF_S_DATA_H			0x03

// dcf data order listing format
#define DCF_CMD_MASK			0x70
#define DCF_COUNTER_MASK		0x0F
#define DCF_PARITY_COUNT		0x80

// all commands with msb set to 1 will store data
#define DCF_CMD_STORE_BIT		0x40

// dcf data order listing commands
#define DCF_CMD_END				0x00
#define DCF_CMD_IGNORE			0x10
#define DCF_CMD_PCHECK			0x20
#define DCF_CMD_ZERO			0x30
#define DCF_CMD_BCD				0x50
#define DCF_CMD_BIN				0x60

// dcf flags
#define DCF_F_ABNORMAL_OPERATION 0x05
#define DCF_F_DST_ANNOUNCEMENT   0x04
#define DCF_F_CEST               0x03
#define DCF_F_CET                0x02
#define DCF_F_LEAP_SECOND        0x01

typedef struct
{
	unsigned char	flags;
	unsigned char	minute;
	unsigned char	hour;
	unsigned char	day;
	unsigned char	weekday;	
	unsigned char	month;
	unsigned char	year;
} dcf_time_t;

extern unsigned char dcf_state;
extern dcf_time_t dcf_time;

unsigned int dcf77_handler(void);
