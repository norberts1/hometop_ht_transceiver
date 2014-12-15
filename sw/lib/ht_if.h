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

#ifndef __HT_IF_H__
#define __HT_IF_H__

#include <avr/io.h>
#include "ht_io.h"
#include "ht_cfg.h"
#include <stdio.h>

typedef struct PIDU_IF_MSG_TYPE {
	uint8_t msgclass;
	uint8_t detail;
	uint8_t option;
} pidu_if_msg_type_t;

typedef struct PIDU_HEADER_MSG {
	uint8_t            starttag;
	pidu_if_msg_type_t msg_type;
	uint8_t            payload_length;
} pidu_if_header_t;

typedef struct  {
	pidu_if_header_t header;
	uint8_t          data[IF_BUFFER_LEN-sizeof(pidu_if_header_t)+1];
} pidu_if_header_data_t;

typedef union {
	uint8_t               msg[IF_BUFFER_LEN+1];
	pidu_if_header_data_t buffer;
} pidu_if_msg_t;

// constants for pidu_if_msg_t.msgclass
const char MSGCLASS_CMD='!';
const char MSGCLASS_ASK='?';
const char MSGCLASS_HT3='H';
const char MSGCLASS_EMS='E';

// constants for pidu_if_msg_t.detail
const char DETAIL_CFG    ='C';
const char DETAIL_BUS_RXD='R';
const char DETAIL_BUS_TXD='S';
const char DETAIL_MODE   ='M';


typedef enum PARSE_STATE 
{
	PARSE_NOTHING = 0,
	SEARCH_HEADER,
	COLLECT_HEADER_BYTES,
	GET_CRC_BYTE,
	COLLECT_DATA_BYTES,
	CALCULATE_CRC,
	DISPATCH_MSG,
} parse_state_t;


class cht_trx_if : public cht_trx_cfg
{
//variables
	public:
	protected:
	private:
		pidu_if_msg_t        m_msg;
		parse_state_t        m_parse_state;
		uint8_t m_headerindex;
		int8_t  m_bytes_countdown;
		uint8_t m_msgcrc;
		uint8_t m_msg_data_index;
		

//functions
	public:
		cht_trx_if();
		~cht_trx_if();
		void init_if(void);
		void send_ht_msg(const uint8_t *p_data, const uint8_t & payload_length);
		int8_t parse_and_dispatch_msg(const uint8_t byte);
		
	protected:
	private:
		cht_trx_if( const cht_trx_if &c );
		cht_trx_if& operator=( const cht_trx_if &c );
		void send_msg_2_master(pidu_if_msg_type_t & msg_type, const uint8_t *p_data, const uint8_t & payload_length);
		void send_msg_2_bus(const uint8_t *p_ifmsg, const uint8_t & msg_length);
		int8_t dispatch_master_msg(void);
}; //piduino_if

#endif //__HT_IF_H__
