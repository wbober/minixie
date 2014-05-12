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

#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <avr/pgmspace.h>

#ifndef LOG_LINE_SEPARATOR
#define LOG_LINE_SEPARATOR "\r\n"
#endif

typedef enum {
	LOG_OFF =   0,
	LOG_FATAL = 1,
	LOG_ERROR = 2,
	LOG_WARN =  3,
	LOG_INFO =  4,
	LOG_DEBUG = 5,
	LOG_TRACE = 6
} log_level_t;

#define ROM_STRING(k,v)   const char k[] PROGMEM = {v}

extern const char LABEL_FATAL[];
extern const char LABEL_ERROR[];
extern const char LABEL_WARN[];
extern const char LABEL_INFO[];
extern const char LABEL_DEBUG[];
extern const char LABEL_TRACE[];

void log_printf(PGM_P level, PGM_P file, uint16_t line, PGM_P fmt, ...);
void log_init(void);

#define _log(_severity, _format, ...) if (LOG_SEVERITY >= LOG_##_severity) log_printf(LABEL_##_severity, PSTR(__FILE__), __LINE__, PSTR(_format), ##__VA_ARGS__)

#define log_fatal(_format, ...) _log(FATAL, _format, ##__VA_ARGS__)
#define log_error(_format, ...) _log(ERROR, _format, ##__VA_ARGS__)
#define log_warn(_format, ...)  _log(WARN,  _format, ##__VA_ARGS__)
#define log_info(_format, ...)  _log(INFO,  _format, ##__VA_ARGS__)
#define log_debug(_format, ...) _log(DEBUG, _format, ##__VA_ARGS__)
#define log_trace(_format, ...) _log(TRACE, _format, ##__VA_ARGS__)


#endif
