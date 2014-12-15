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

#include "ht_cfg.h"
#include "ht_process.h"
#include "ht_io.h"
#include "ht_error.h"



volatile ht_cfg_t g_ht_cfg;

/* configuration-parameter stored to EEPROM */
uint8_t EEMEM eep_run_mode      = RUN_MODE_RX_TX;   /* default runmode */
uint8_t EEMEM eep_slave_address = HT_ADDRESS_MODEM; /* default device - address */
uint8_t EEMEM eep_values_valid  = 1;                /* flag to decide for using these eep-values */


cht_trx_cfg::cht_trx_cfg()
{
	g_ht_cfg.run_mode      = RUN_MODE_NATIVE;
	g_ht_cfg.our_address   = HT_ADDRESS_NO_RESPONSE;
}

cht_trx_cfg::~cht_trx_cfg()
{
	
}

uint8_t cht_trx_cfg::readconfig(void)
{
	uint8_t fktrtn=ERR_NONE;
	// set and read testpins TP-OUT and TP-IN to get startmode informations at startup
	//  if TP-OUT and TP-IN are connected at startup-time then native mode is read
	//    Native-mode is only available, if three times TP-Out high and three times TP-Out low
	//    result in value tp_in=6
	SET_TP_OUT(ON);
	asm("nop");
	uint8_t tp_value=GETSTATUS_TP_IN;
	uint8_t tp_in=0;
	if (tp_value == 1) { tp_in+=1; }
	SET_TP_OUT(OFF);
	asm("nop");
	tp_value=GETSTATUS_TP_IN;
	if (tp_value == 0) { tp_in+=1; }
	SET_TP_OUT(ON);
	asm("nop");
	tp_value=GETSTATUS_TP_IN;
	if (tp_value == 1) { tp_in+=1; }
	SET_TP_OUT(OFF);
	asm("nop");
	tp_value=GETSTATUS_TP_IN;
	if (tp_value == 0) { tp_in+=1; }
	SET_TP_OUT(ON);
	asm("nop");
	tp_value=GETSTATUS_TP_IN;
	if (tp_value == 1) { tp_in+=1; }
	SET_TP_OUT(OFF);
	asm("nop");
	tp_value=GETSTATUS_TP_IN;
	if (tp_value == 0) { tp_in+=1; }
	CLEAR_TP_OUT;
	
	if(tp_in == 6) {
		g_ht_cfg.run_mode      = RUN_MODE_NATIVE;
	} else {
		/* check first for EEProm values. if not available or invalid, set defaults */
		uint8_t valid=eeprom_read_byte(&eep_values_valid);
		if(valid == 1) {
			uint8_t rntvalue=eeprom_read_byte(&eep_run_mode);
			(rntvalue != 0xff ) ? g_ht_cfg.run_mode = rntvalue : g_ht_cfg.run_mode = RUN_MODE_RX_TX;
			rntvalue=eeprom_read_byte(&eep_slave_address);
			if((rntvalue <= HT_ADDRESS_NO_RESPONSE) && rntvalue >= 0x0a) {
				g_ht_cfg.our_address = rntvalue;
			} else {
				g_ht_cfg.our_address = HT_ADDRESS_MODEM;
			}
		} else {
			// eeprom values invalid, set defaults
			g_ht_cfg.run_mode    = RUN_MODE_NATIVE;
		}
	}
	// fource slave-address to NO_RESPONSE, if 'tx' is not enabled
	if(IsModeNative() || IsModeRx_Header() || g_ht_cfg.run_mode==0) {
		g_ht_cfg.our_address = HT_ADDRESS_NO_RESPONSE;
	}
	
	return fktrtn;
}

uint8_t cht_trx_cfg::writeconfig(uint8_t config_type, uint8_t config_value)
{
	uint8_t fktrtn=ERR_WRONG_VALUE;
	switch (config_type)
	{
		//System-Startmode
		case 1:
		{
			/* write mode-value to eeprom */
			if((config_value < RUN_MODE_NATIVE) || (config_value==0xff)) {
				//writing new mode to EEProm and check the result
				eeprom_update_byte(&eep_run_mode, config_value);
				eeprom_busy_wait();
				uint8_t rtnvalue=eeprom_read_byte(&eep_run_mode);
				if (rtnvalue==config_value) {
					eeprom_update_byte(&eep_values_valid, 1);
					eeprom_busy_wait();
					fktrtn=ERR_NONE;
				}	else {
					fktrtn=ERR_CFG_WRITE;
				}
			}
		}
		break;
		
		//transceiver device-address
		case 2:
		{
			//first check for valid byte
			if((config_value <= HT_ADDRESS_NO_RESPONSE) && config_value >= 0x0a) {
				//second write to EEProm and check the result
				eeprom_update_byte(&eep_slave_address, config_value);
				eeprom_busy_wait();
				uint8_t rtnvalue=eeprom_read_byte(&eep_slave_address);
				if (rtnvalue==config_value) {
					eeprom_update_byte(&eep_values_valid, 1);
					eeprom_busy_wait();
					fktrtn=ERR_NONE;
				}	else {
					fktrtn=ERR_CFG_WRITE;
				}
			}
		}
		break;
		default:
			fktrtn=ERR_WRONG_VALUE;
		break;
	}
	return fktrtn;
}

uint8_t cht_trx_cfg::readconfig(uint8_t config_type, uint8_t * const p_config_value)
{
	uint8_t fktrtn=ERR_WRONG_VALUE;
	uint8_t valid=0;
	switch (config_type)
	{
		//System-Startmode
		case 1:
		{
				*p_config_value=eeprom_read_byte(&eep_run_mode);
				valid=eeprom_read_byte(&eep_values_valid);
				(valid == 1) ? fktrtn=ERR_NONE : fktrtn=ERR_INVALID_VALUE;
		}
		break;
		//transceiver device-address
		case 2:
		{
				*p_config_value=eeprom_read_byte(&eep_slave_address);
				valid=eeprom_read_byte(&eep_values_valid);
				(valid == 1) ? fktrtn=ERR_NONE : fktrtn=ERR_INVALID_VALUE;
		}
		break;
		default:
			fktrtn=ERR_WRONG_VALUE;
		break;
	}
	return fktrtn;
}


uint8_t cht_trx_cfg::IsModeNative(void) {
	return ((g_ht_cfg.run_mode == RUN_MODE_NATIVE) ? 1:0);
}

uint8_t cht_trx_cfg::IsModeRx_Header(void) {
	return ((g_ht_cfg.run_mode == RUN_MODE_RX_HEADER) ? 1:0);
}

uint8_t cht_trx_cfg::IsModeTx(void) {
	return ((g_ht_cfg.run_mode == RUN_MODE_TX) ? 1:0);
}

uint8_t cht_trx_cfg::IsModeRx_Tx(void) {
	return ((g_ht_cfg.run_mode == RUN_MODE_RX_TX) ? 1:0);
}
