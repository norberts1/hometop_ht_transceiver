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

#ifndef __HT_CFG_H__
#define __HT_CFG_H__

#include <avr/io.h>
#include <avr/eeprom.h>

typedef enum eRUN_MODE{
	RUN_MODE_NATIVE   =0x04, /* Bus-rx send to master without header, no tx */
	RUN_MODE_RX_HEADER=0x01, /* Bus-rx send to master with header */
	RUN_MODE_TX       =0x02, /* Bus-tx active */
	RUN_MODE_RX_TX    =0x03  /* Bus-rx with header and tx active */
} eRUN_MODE_t;

typedef enum eHT_ADDRESS {
	HT_ADDRESS_COMPUTER   =0x0b,
	HT_ADDRESS_MODEM      =0x0d,
	HT_ADDRESS_NO_RESPONSE=0x7f
} eHT_ADDRESS_t;

class cht_trx_cfg {
//variables
	private:
	
//functions
	public:
		cht_trx_cfg();
		~cht_trx_cfg();
		uint8_t readconfig(void);
		uint8_t writeconfig(uint8_t config_type, uint8_t config_value);
		uint8_t readconfig (uint8_t config_type, uint8_t * const p_config_value);
		uint8_t IsModeNative(void);
		uint8_t IsModeRx_Header(void);
		uint8_t IsModeTx(void);
		uint8_t IsModeRx_Tx(void);
	
	private:
		cht_trx_cfg( const cht_trx_cfg &c );
		cht_trx_cfg& operator=( const cht_trx_cfg &c );
};

#endif /* __HT_CFG_H__ */
