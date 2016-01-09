#! /bin/sh
#
 ##
 # #################################################################
 ## Copyright (c) 2016 Norbert S. <junky-zs@gmx.de>
 #
 # This program is free software: you can redistribute it and/or modify
 # it under the terms of the GNU General Public License as published by
 # the Free Software Foundation, either version 3 of the License, or
 # (at your option) any later version.
 #
 # This program is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 # GNU General Public License for more details.
 #
 # You should have received a copy of the GNU General Public License
 # along with this program. If not, see <http://www.gnu.org/licenses/>.
 #
 #################################################################
 # Ver:0.1    /Datum 09.01.2016
#
echo "------------------------------------------------------------------------------------"
echo "prepare 'ht_pitiny-board' with current software (setting fuses, flash and eeprom)"
echo "------------------------------------------------------------------------------------"
echo "Set fuses to values: Ext=F5;High=D5;Low=EE"
sudo avrdude -cgpio -pattiny841 -U lfuse:w:0xEE:m -U hfuse:w:0xD5:m -U efuse:w:0xF5:m
echo "------------------------------------------------------------------------------------"
echo "Write programm to flash"
sudo avrdude -cgpio -v -pattiny841 -Uflash:w:./pitiny/pitiny/Release/pitiny.hex
echo "------------------------------------------------------------------------------------"
sudo avrdude -cgpio -v -pattiny841 -Ueeprom:w:./pitiny/pitiny/Release/pitiny.eep
echo "preparing of 'ht_pitiny-board' done"
echo "------------------------------------------------------------------------------------"

