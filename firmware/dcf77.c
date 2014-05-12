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

#include <avr/io.h>
#include "dcf77.h"

unsigned char bcd_weigts[8] = {1,2,4,8,10,20,40,80};

dcf_time_t dcf_time;
unsigned char dcf_state = DCF_S_WAIT;

// list of bytes that define how to decode dcf bit stream
//
// The following format is used for the command bytes:
// bit		function
// 0..3		number of bits
// 4..6		decode command
// 7		count parity for bits

unsigned char dcf_cmd_list[] =
{
	DCF_CMD_ZERO	| 1,
	DCF_CMD_IGNORE	| 14,
	DCF_CMD_BIN		| 6	,
	DCF_CMD_BCD		| 7		|	DCF_PARITY_COUNT,
	DCF_CMD_PCHECK	| 1	,
	DCF_CMD_BCD		| 6		|	DCF_PARITY_COUNT,
	DCF_CMD_PCHECK	| 1	,	
	DCF_CMD_BCD		| 6		|	DCF_PARITY_COUNT,
	DCF_CMD_BCD		| 3		|	DCF_PARITY_COUNT,	
	DCF_CMD_BCD		| 5		|	DCF_PARITY_COUNT,	
	DCF_CMD_BCD		| 8		|	DCF_PARITY_COUNT,	
	DCF_CMD_PCHECK	| 1	,
	DCF_CMD_END
};

extern unsigned short timer;

unsigned int dcf77_handler(void)
{
	static unsigned char parity,data,pos,cnt,*cmd;

	static dcf_time_t dcf_time_buff;
	static unsigned char *time_output;
	unsigned char dcf_time_ok = 0;

	static unsigned short last_time = 0;
	unsigned short period_timer = (timer | TCNT2) - last_time;


	unsigned char next_dcf_state = dcf_state;
	switch(dcf_state)
	{
		// reading data, dcf bit is low, waiting to get high to get next bit
		case DCF_S_DATA_L:
			// if we have waited too long, abort and wait for next sync and try again				
			if(period_timer > DCF_TIME_L_MAX)
				next_dcf_state = DCF_S_ERROR;
			
			// if we get a high bit start reading data
			if(dcf_pin)
				next_dcf_state = DCF_S_DATA_H;
				
			break;


		// reading data, dcf bit is high, waiting to get high to get low to determine if 1 or 0
		case DCF_S_DATA_H:
			if(period_timer > DCF_TIME_H_MAX)
				next_dcf_state = DCF_S_ERROR;
				
			if(!dcf_pin)
			{
				if(period_timer < DCF_TIME_L_MIN)
					next_dcf_state = DCF_S_ERROR;
				else
				{
					unsigned char bit = (period_timer > DCF_TIME_L);
					next_dcf_state = DCF_S_DATA_L;
					
					if(*cmd & DCF_PARITY_COUNT)
						parity+=bit;

					switch(*cmd & DCF_CMD_MASK)
					{
						case DCF_CMD_BCD:
							data += bit?bcd_weigts[pos]:0; //bcd - each bit shall be weighted
							pos++;
							break;

						case DCF_CMD_BIN:
							data = (data<<1) | bit;	// binary - just shift in the bits
							break;

						case DCF_CMD_PCHECK:
							if((parity&1)!=bit)		// check if parity is same as bit
								next_dcf_state = DCF_S_ERROR;
							parity=0;
							break;

						case DCF_CMD_ZERO:
							if(bit)				// must be zero
								next_dcf_state = DCF_S_ERROR;
							break;

						case DCF_CMD_IGNORE:	// if bit is to be ignored, do nothing
						default:
							break;
					}
					
					if(!--cnt)
					{
						if(*cmd & DCF_CMD_STORE_BIT)
							*(time_output++) = data;	// write data and point at next position
							
						cmd++;								// point at next command byte
						cnt		= *cmd & DCF_COUNTER_MASK;	// get number of bits for this command to read
						data	= 0;						// clear data buffer to read next byte
						pos		= 0;
							
						if((*cmd & DCF_CMD_MASK) == DCF_CMD_END)	// if time info was completely read
						{
							dcf_time_ok = 1;			    // keep track on last dcf time update
							dcf_time = dcf_time_buff;		// then store successful result
							next_dcf_state = DCF_S_WAIT;	// and start reading next time information
						}
					}
				}	
			}
			break;
			
		// sync, dcf bit is low, waiting for it to get high
		case DCF_S_SYNC:
		
			if(dcf_pin) // if dcf gets high and low period was long enough
			{
				if(period_timer > DCF_TIME_SYNC_MIN)
				{
					next_dcf_state = DCF_S_DATA_H;
					parity	= 0;
					cmd = dcf_cmd_list;
					cnt = *cmd & DCF_COUNTER_MASK;
					time_output = (unsigned char *) &dcf_time_buff.flags;
					
				}
				else
					next_dcf_state = DCF_S_WAIT;
			}
			break;

		case DCF_S_WAIT:
		default:		
			if(!dcf_pin)
				next_dcf_state = DCF_S_SYNC;
	}
	
	if(dcf_state != next_dcf_state)
	{
		dcf_state = next_dcf_state;
		last_time = timer | TCNT2;
	}

	return dcf_time_ok;
}
