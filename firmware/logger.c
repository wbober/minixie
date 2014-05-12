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
 */

#include <stdio.h>
#include "uart.h"
#include "logger.h"


ROM_STRING(LABEL_FATAL, "FATAL");
ROM_STRING(LABEL_ERROR, "ERROR");
ROM_STRING(LABEL_WARN,  "WARN");
ROM_STRING(LABEL_INFO,  "INFO");
ROM_STRING(LABEL_DEBUG, "DEBUG");
ROM_STRING(LABEL_TRACE, "TRACE");

static int put(char c, FILE *stream)
{
	return uart_write(UART0, (uint8_t *)&c, 1);
}

static int get(FILE *stream)
{
	uint8_t c;
	return uart_read(UART0, &c, 1);
}

void log_init(void)
{
	fdevopen(put, get);
}
/**
    Print debug info to terminal.
 */
void log_printf(PGM_P severity, PGM_P file, uint16_t line, PGM_P fmt, ...)
{
    va_list ap;
    fprintf_P(stdout, PSTR("%S %S:%d "), severity, file, line);
    va_start(ap, fmt);
    vfprintf_P(stdout, fmt, ap);
    va_end(ap);
    fprintf_P(stdout, PSTR(LOG_LINE_SEPARATOR));
}
