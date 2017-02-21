/* bricklib2
 * Copyright (C) 2010-2016 Olaf Lüke <olaf@tinkerforge.com>
 * Copyright (C) 2017 Ishraq Ibne Ashraf <ishraq@tinkerforge.com>
 *
 * util_definitions.h: General useful macros
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

#ifndef UTIL_H
#define UTIL_H

#ifndef ABS
	#define ABS(a) (((a) < 0) ? (-(a)) : (a))
#endif
#ifndef MIN
	#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
	#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef BETWEEN
	#define BETWEEN(min, value, max)  (MIN(max, MAX(value, min)))
#endif

#define SCALE(val_a, min_a, max_a, min_b, max_b) \
	(((((val_a) - (min_a))*((max_b) - (min_b)))/((max_a) - (min_a))) + (min_b))

#ifndef HTONS
	#define HTONS(x) ( ((x & 0xFF) << 8) | (((x) >> 8) & 0xFF) )
#endif

#ifndef NTOHS
	#define NTOHS(x) HTONS(x)
#endif

#ifndef HTONL
	#define HTONL(x) ( ((x) << 24 & 0xFF000000UL) | \
	                   ((x) << 8 & 0x00FF0000UL) | \
	                   ((x) >> 8 & 0x0000FF00UL) | \
	                   ((x) >> 24 & 0x000000FFUL) )
#endif

#ifndef NTOHL
	#define NTOHL(x) HTONL(x)
#endif

#endif
