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

#include "ht_if.h"
#include "ht_process.h"
#include "ht_error.h"
#include "ht_cfg.h"
#include <string.h>
#include <util/atomic.h>


//load additonal include-file only for ATtiny
#ifdef PITINY
	#include "pitiny_uart.h"
#endif

// default constructor
cht_trx_if::cht_trx_if()
{
	m_parse_state    = PARSE_NOTHING;
	m_bytes_countdown= IF_BUFFER_LEN-1;
} //cht_trx_if

// default destructor
cht_trx_if::~cht_trx_if()
{
} //~cht_trx_if


//public functions
void cht_trx_if::init_if(void)
{
	cht_trx_cfg::readconfig();
}

void cht_trx_if::send_ht_msg(const uint8_t *p_data, const uint8_t & payload_length)
{
	pidu_if_msg_type_t msg_type;
	msg_type.msgclass = MSGCLASS_HT3;
	msg_type.detail   = DETAIL_BUS_RXD;
	msg_type.option   = 0x11; //1 packet max and 1 is current packet;
	this->send_msg_2_master(msg_type, p_data, payload_length);
}


int8_t cht_trx_if::parse_and_dispatch_msg(const uint8_t byte)
{
#ifdef DEBUG
	char tmpbuffer[32];
	sprintf(tmpbuffer, ">S:%d;bc:%d;BY:%0X\r",m_parse_state,m_bytes_countdown,byte);
	#ifndef PITINY
		swuart_puts(tmpbuffer);
	#else		
		tiny_uart_puts(tmpbuffer, UART0);
	#endif
#endif	
	/*
	 * Msg-Header is:
	 *     0          1         2        3         4            5 ff      N   N+1 
	 *  start-tag  msgclass  detail   option  payload_length ->[data-bytes]   crc
	 *    or
	 *  start-tag  msgclass  detail   option       0           crc
	*/
	if (m_parse_state == PARSE_NOTHING)	
	{
		m_headerindex     = 0;
		m_bytes_countdown = sizeof(pidu_if_header_t);
		m_parse_state     = SEARCH_HEADER;
		m_msg_data_index  = 0;
		m_msgcrc          = 0;
	}
	
	if (m_parse_state == GET_CRC_BYTE)
	{
		m_msgcrc = byte;
		m_parse_state = CALCULATE_CRC;
	}
	
	if (m_parse_state == COLLECT_DATA_BYTES)	
	{
		// check index, if failed end handling
		if (m_msg_data_index > IF_BUFFER_LEN-1) {
			m_parse_state = PARSE_NOTHING; 
			m_bytes_countdown = ERR_COMMON;
		} else {
			// save data to buffer
			m_msg.buffer.data[m_msg_data_index]=byte;
			m_msg_data_index++;
			m_bytes_countdown--;
			if(m_bytes_countdown <= 1) {
				//data-bytes end, goto CRC-check
				m_parse_state = GET_CRC_BYTE;
				m_bytes_countdown=0;
			}
		}
	}
	
	if (m_parse_state == COLLECT_HEADER_BYTES)	
	{
		//collect bytes for complete header
		m_headerindex++;
		if(m_headerindex < sizeof(pidu_if_header_t))	{
			m_msg.msg[m_headerindex]=byte;
		}
		m_bytes_countdown--;
		if(m_headerindex >= 4) 
		{
			// check for datalength in byte 4
			if(m_msg.buffer.header.payload_length == 0) {
				// header only message, go to crc check state
				m_parse_state = GET_CRC_BYTE;
				m_bytes_countdown = 0;
			} else {
				// msg-length is: amount of databytes and trailing crc
				m_bytes_countdown=m_msg.buffer.header.payload_length + 1;
				if(m_bytes_countdown >= (IF_BUFFER_LEN-4)) {
					//datalength error release collecting state
					m_bytes_countdown = ERR_COMMON;
					m_parse_state = PARSE_NOTHING;
				} else {
					// message with data and crc, go to data collecting state
					m_parse_state = COLLECT_DATA_BYTES;
				}
			}
			m_headerindex=0;
			if(m_bytes_countdown < 0) {
				m_parse_state = PARSE_NOTHING;
			}
		} //end if(m_headerindex == 4)
	} // state == COLLECT_HEADER_BYTES
	
	
	if (m_parse_state == SEARCH_HEADER)	
	{
		if (byte == '#') {
			m_headerindex   =0;
			m_msg.msg[m_headerindex]=byte;
			m_parse_state = COLLECT_HEADER_BYTES;
			m_bytes_countdown = sizeof(pidu_if_header_t)-1;
		} else {
			m_bytes_countdown--;
			if (m_bytes_countdown == 0) {
				// go back to passive state
				m_parse_state = PARSE_NOTHING;
			}
		}
	}
	if (m_parse_state == CALCULATE_CRC)	
	{
		// calculate crc and compare it
		uint8_t calc_crc=ht_process_calc_checksum(m_msg.msg, sizeof(pidu_if_header_t)+m_msg.buffer.header.payload_length);
		if (calc_crc == m_msgcrc) {
			m_parse_state = DISPATCH_MSG;
			m_bytes_countdown=0;
		} else {
			// send error-response (#??<errorcode>0crc)
			pidu_if_msg_type_t msg_response;
			msg_response.msgclass = '?';
			msg_response.detail   = '?';
			msg_response.option   = ERR_CRC;
			m_msg.buffer.header.payload_length=0;
			send_msg_2_master(msg_response, NULL, sizeof(pidu_if_header_t));
			// go back to passive state
			m_parse_state = PARSE_NOTHING;
			m_bytes_countdown = ERR_COMMON;
		}
	}
	if (m_parse_state == DISPATCH_MSG)	
	{
		//call dispatcher for msg-handling
		dispatch_master_msg();
		//go back to passive state
		m_parse_state = PARSE_NOTHING;
		m_bytes_countdown=0;
	}
#ifdef DEBUG	
	sprintf(tmpbuffer, "<S:%d;bc:%d\r",m_parse_state,m_bytes_countdown);
	#ifndef PITINY
		swuart_puts(tmpbuffer);
	#else
		tiny_uart_puts(tmpbuffer, UART0);
	#endif
#endif	
	return m_bytes_countdown;
}


//private functions
void cht_trx_if::send_msg_2_bus(const uint8_t *p_ifmsg, const uint8_t & msg_length)
{
	
}

void cht_trx_if::send_msg_2_master(pidu_if_msg_type_t & msg_type, const uint8_t *p_data, const uint8_t & payload_length)
{
	uint8_t crc=0;
	uint8_t uindex=0;
	uint8_t msg_buffer[IF_BUFFER_LEN];
	// setup header-data to buffer
	msg_buffer[0] = '#';
	msg_buffer[1] = msg_type.msgclass;
	msg_buffer[2] = msg_type.detail;
	msg_buffer[3] = msg_type.option;
	msg_buffer[4] = payload_length;
	uint8_t msglength=sizeof(pidu_if_header_t)+payload_length;
	if(payload_length && payload_length<(IF_BUFFER_LEN-sizeof(pidu_if_header_t))) {
		//write data to send-buffer
		memcpy(&msg_buffer[sizeof(pidu_if_header_t)], p_data, payload_length);
	}
	//calculate crc for msg-bytes: Header+data
	crc=ht_process_calc_checksum(msg_buffer, msglength);
	//send data to master
#ifndef PITINY
	for(uindex=0; uindex < msglength; uindex++) {
		swuart_putchar(msg_buffer[uindex]);
	}
	//send crc at least
	swuart_putchar(crc);
#else
	for(uindex=0; uindex < msglength; uindex++) {
		tiny_uart_putchar(msg_buffer[uindex], UART0);
	}
	//send crc at least
	tiny_uart_putchar(crc, UART0);
#endif	
}

int8_t cht_trx_if::dispatch_master_msg(void)
{
	int8_t fktrtn=0;
	pidu_if_msg_type_t msg_response;
	uint8_t msg_response_length=0;
	
#ifdef DEBUG
		char tmpbuffer[32];
		sprintf(tmpbuffer, "Dispatch_S:%d;bc:%d\r",m_parse_state,m_bytes_countdown);
		uint8_t length=strlen(tmpbuffer);
	#ifndef PITINY
		// print header
		swuart_putchar('#');
		swuart_putchar('D');
		swuart_putchar('L');
		swuart_putchar(17);
		swuart_putchar(length);
		//print data
		swuart_puts(tmpbuffer);
	#else
		// print header
		tiny_uart_putchar('#', UART0);
		tiny_uart_putchar('D', UART0);
		tiny_uart_putchar('L', UART0);
		tiny_uart_putchar(17, UART0);
		tiny_uart_putchar(length, UART0);
		//print data
		tiny_uart_puts(tmpbuffer, UART0);
	#endif
#endif	
	// check 1. Byte of header (starttag)
	if (this->m_msg.buffer.header.starttag != '#') {
		// error, nothing to do
		fktrtn = ERR_WRONG_TAG;
		msg_response.option   = (uint8_t)fktrtn;
		msg_response_length   = 0;
	} else {
		// check 2. Byte of header (messageclass)
		switch(m_msg.buffer.header.msg_type.msgclass)
		{
			//-------------------------------
			case MSGCLASS_CMD:
			{
				// check 3. Byte of header (messagedetail)
				switch(m_msg.buffer.header.msg_type.detail)
				{
					case DETAIL_CFG:
					{
						if(m_msg.buffer.header.payload_length == 1) {
							fktrtn = cht_trx_cfg::writeconfig(m_msg.buffer.header.msg_type.option, m_msg.buffer.data[0]);
						} else {
							fktrtn = ERR_WRONG_MSG;
						}
						msg_response.option   = (uint8_t)fktrtn;
						msg_response_length   = 0;
					}
					break;
					case DETAIL_BUS_TXD:
					{ 
						//copy data to ht_processing
						fktrtn=ht_process_txdata(&m_msg.buffer.data[0], m_msg.buffer.header.payload_length);
#ifdef DEBUG
						sprintf(tmpbuffer,"BUS_TX;Size:<%d>;rtn:<%d>\r",m_msg.buffer.header.payload_length,fktrtn);
						#ifndef PITINY
							swuart_puts(tmpbuffer);
						#else
							tiny_uart_puts(tmpbuffer, UART0);
						#endif
#endif	
						msg_response.option   = (uint8_t)fktrtn;
						msg_response_length   = 0;
					}
					break;
					case DETAIL_MODE:
					{
						if (m_msg.buffer.header.msg_type.option <= RUN_MODE_NATIVE) {
							// write mode temporarly to system
							g_ht_cfg.run_mode = m_msg.buffer.header.msg_type.option;
							// fource slave-address to NO_RESPONSE, if 'tx' is not enabled
							if(cht_trx_cfg::IsModeNative() || cht_trx_cfg::IsModeRx_Header() || m_msg.buffer.header.msg_type.option==0) {
								ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
									g_ht_cfg.our_address = HT_ADDRESS_NO_RESPONSE;
								}
							}
							fktrtn = ERR_NONE;
						} else 
							if (m_msg.buffer.header.msg_type.option==0xf0) 
							{
								// FORCE reset transceiver-adapter
								// disable any further interrupts
								cli();
								// Enable watchdog for reset with shortest time
								WDTCSR = _BV(WDE);
								fktrtn = ERR_NONE;
								// wait forever
								while (1) {
									OFF_STATUS_LED;
								}
							} else {
								fktrtn = ERR_WRONG_VALUE;
							}
						msg_response.option   = (uint8_t)fktrtn;
						msg_response_length   = 0;
					}
					break;
					default:
#ifdef DEBUG
						sprintf(tmpbuffer,"Unknown Cmd:Detail:<%c>\r",m_msg.buffer.header.msg_type.detail);
						#ifndef PITINY
							swuart_puts(tmpbuffer);
						#else
							tiny_uart_puts(tmpbuffer, UART0);
						#endif
#endif					
						fktrtn = ERR_WRONG_VALUE;
						msg_response.option   = (uint8_t)fktrtn;
						msg_response_length   = 0;
					break;
				}
			} //end MSGCLASS_CMD
			break;
			//-------------------------------
			case MSGCLASS_ASK:
			{
				// check 3. Byte of header (messagedetail)
				switch(m_msg.buffer.header.msg_type.detail)
				{
					case DETAIL_MODE:
					{
						fktrtn = ERR_NONE;
						msg_response.option   = g_ht_cfg.run_mode;
						msg_response_length   = 0;
					}
					break;
					case DETAIL_CFG:
					{
						if(m_msg.buffer.header.payload_length == 0) {
							fktrtn = cht_trx_cfg::readconfig(m_msg.buffer.header.msg_type.option, &m_msg.buffer.data[0]);
						} else {
							fktrtn = ERR_WRONG_MSG;
						}
						msg_response.option   = (uint8_t)fktrtn;
						msg_response_length   = 1;
					}
					break;
					default:
						fktrtn = ERR_WRONG_VALUE;
						msg_response_length   = 0;
					break;
				}
			} //end MSGCLASS_ASK
			break;
			//-------------------------------
			default:
#ifdef DEBUG
				sprintf(tmpbuffer,"Cmd:<%c> unknown\r",m_msg.buffer.header.msg_type.msgclass);
				#ifndef PITINY
					swuart_puts(tmpbuffer);
				#else
					tiny_uart_puts(tmpbuffer, UART0);
				#endif
#endif			
				fktrtn = ERR_WRONG_VALUE;
				msg_response.option   = (uint8_t)fktrtn;
				msg_response_length   = 0;
			break;
		} //end switch((m_msg.buffer.header.msg_type.msgclass))
	} //end if (this->m_msg.buffer.header.starttag != '#')
	
	//send answer to master (option has to be set according to fkt-result)
	//response-detail with lower case letters
	msg_response.msgclass = m_msg.buffer.header.msg_type.msgclass;
	msg_response.detail   = m_msg.buffer.header.msg_type.detail + 0x20;
	send_msg_2_master(msg_response,  &m_msg.buffer.data[0], msg_response_length);
	return fktrtn;
}
