/* bricklib2
 * Copyright (C) 2010-2016 Olaf Lüke <olaf@tinkerforge.com>
 *
 * logging.h: Logging functionality for lcd and serial console
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef LOGGING_H
#define LOGGING_H

#include "configs/config_logging.h"
#include <stdio.h>
#include <string.h>

#define LOGGING_DEBUG    0
#define LOGGING_INFO     1
#define LOGGING_WARNING  2
#define LOGGING_ERROR    3
#define LOGGING_FATAL    4
#define LOGGING_NONE     5

#ifndef LOGGING_LEVEL
#define LOGGING_LEVEL LOGGING_NONE
#endif

#ifdef LOGGING_SDCARD
#include "sdlog.h"
#else
#define sdlog_printf(...) {}
#endif

#ifdef LOGGING_UARTBB
#include "bricklib2/hal/uartbb/uartbb.h"
#define platform_printf uartbb_printf
#else
#define platform_printf printf
#endif

void logging_init(void);

#ifdef COMPILE_FOR_RELEASE
#define LOGGING_PRINT(...) \
	do{ \
		sdlog_printf(__VA_ARGS__); \
	} while(0)
#else
#define LOGGING_PRINT(...) \
	do{ \
		platform_printf(__VA_ARGS__); \
		sdlog_printf(__VA_ARGS__); \
	} while(0)
#endif

#ifdef LOGGING_USE_BASENAME
#define LOGGING_BASENAME(X) strrchr(X, '/') + 1
#else
#define LOGGING_BASENAME(X) X
#endif

#ifdef LOGGING_HAVE_SYSTEM_TIME
#include LOGGING_SYSTEM_TIME_HEADER
#ifndef LOGGING_TIMESTAMP_FORMAT
#define LOGGING_TIMESTAMP_FORMAT "%lu "
#endif
#define LOGGING_TIMESTAMP_VALUE LOGGING_SYSTEM_TIME_FUNCTION(),
#else
#define LOGGING_TIMESTAMP_FORMAT ""
#define LOGGING_TIMESTAMP_VALUE
#endif

#ifdef COMPILE_FOR_RELEASE
#define logd(str,  ...) {}
#define logwohd(str,  ...) {}
#elif LOGGING_LEVEL <= LOGGING_DEBUG
#define logd(str,  ...) do{ LOGGING_PRINT("<" LOGGING_TIMESTAMP_FORMAT "D %s:%d> " str, LOGGING_TIMESTAMP_VALUE LOGGING_BASENAME(__FILE__), __LINE__, ##__VA_ARGS__); }while(0)
#define logwohd(str,  ...) do{ LOGGING_PRINT(str, ##__VA_ARGS__); }while(0)
#else
#define logd(str,  ...) {}
#define logwohd(str,  ...) {}
#endif

#if LOGGING_LEVEL <= LOGGING_INFO
#define logi(str,  ...) do{ LOGGING_PRINT("<" LOGGING_TIMESTAMP_FORMAT "I %s:%d> " str, LOGGING_TIMESTAMP_VALUE LOGGING_BASENAME(__FILE__), __LINE__, ##__VA_ARGS__); }while(0)
#define logwohi(str,  ...) do{ LOGGING_PRINT(str, ##__VA_ARGS__); }while(0)
#else
#define logi(str,  ...) {}
#define logwohi(str,  ...) {}
#endif

#if LOGGING_LEVEL <= LOGGING_WARNING
#define logw(str,  ...) do{ LOGGING_PRINT("<" LOGGING_TIMESTAMP_FORMAT "W %s:%d> " str, LOGGING_TIMESTAMP_VALUE LOGGING_BASENAME(__FILE__), __LINE__, ##__VA_ARGS__); }while(0)
#define logwohw(str,  ...) do{ LOGGING_PRINT(str, ##__VA_ARGS__); }while(0)
#else
#define logw(str,  ...) {}
#define logwohw(str,  ...) {}
#endif

#if LOGGING_LEVEL <= LOGGING_ERROR
#define loge(str,  ...) do{ LOGGING_PRINT("<" LOGGING_TIMESTAMP_FORMAT "E %s:%d> " str, LOGGING_TIMESTAMP_VALUE LOGGING_BASENAME(__FILE__), __LINE__, ##__VA_ARGS__); }while(0)
#define logwohe(str,  ...) do{ LOGGING_PRINT(str, ##__VA_ARGS__); }while(0)
#else
#define loge(str,  ...) {}
#define logwohe(str,  ...) {}
#endif

#if LOGGING_LEVEL <= LOGGING_FATAL
#define logf(str,  ...) do{ LOGGING_PRINT("<" LOGGING_TIMESTAMP_FORMAT "F %s:%d> " str, LOGGING_TIMESTAMP_VALUE LOGGING_BASENAME(__FILE__), __LINE__, ##__VA_ARGS__); }while(0)
#define logwohf(str,  ...) do{ LOGGING_PRINT(str, ##__VA_ARGS__); }while(0)
#else
#define logf(str,  ...) {}
#define logwohf(str,  ...) {}
#endif

#if(DEBUG_STARTUP)
#define logsd(str, ...) do{logd("s: " str, ##__VA_ARGS__);}while(0)
#define logsi(str, ...) do{logi("s: " str, ##__VA_ARGS__);}while(0)
#define logsw(str, ...) do{logw("s: " str, ##__VA_ARGS__);}while(0)
#define logse(str, ...) do{loge("s: " str, ##__VA_ARGS__);}while(0)
#define logsf(str, ...) do{logf("s: " str, ##__VA_ARGS__);}while(0)
#else
#define logsd(str, ...) {}
#define logsi(str, ...) {}
#define logsw(str, ...) {}
#define logse(str, ...) {}
#define logsf(str, ...) {}
#endif

#endif
