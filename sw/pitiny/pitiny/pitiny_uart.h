/*
 * Copyright (c) 2013 Norbert S. <junky-zs@gmx.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __PITINY_UART_H__
#define __PITINY_UART_H__

/* Enable c-linkage */
#if defined (__cplusplus)
extern "C" {
	#endif

#include "ht_io.h"

#define UART0 0
#define UART1 1

void tiny_uart_init( const uint8_t uart_nr, const unsigned int baud  );
void tiny_uart_putchar   ( const uint8_t val, const uint8_t uart_nr );
void tiny_uart_puts      ( const char *txt  , const uint8_t uart_nr );
uint8_t tiny_uart_getchar( const uint8_t uart_nr );


uint8_t tiny_uart_char_received(void);


#if defined (__cplusplus)
} //extern "C"
#endif


#endif