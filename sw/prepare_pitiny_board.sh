#! /bin/sh
#
 ##
 # #################################################################
 ## Copyright (c) 2016 Norbert S. <junky-zs at gmx dot de>
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
 # Ver:0.2    /Datum 04.03.2023 update for 'linuxgpio'-port and installation-
 #                              handling.
 # Ver:0.3    /Datum 09.03.2023 path-correction
 #################################################################
#
# check platform (e.g. Raspberry Pi) for sw-update.
#   only Embedded Linux platforms are allowed.

# save current path
running_path=$(pwd)

echo "------------------------------------------------------------------------------------"
echo "Check platform type";
machine=$(uname -m)
if expr "${machine}" : '^\(arm\|aarch\)' >/dev/null
  then
    echo " -> OK"
  else
    echo " -> 'ht_pitiny' software-installation is not possible on this platform";
    exit 1
fi
echo "------------------------------------------------------------------------------------"
echo "Prepare 'system' for software-installation"
echo "------------------------------------------------------------------------------------"
sudo -v >/dev/null
if [ $? -ne 0 ]; then
 echo " -> sudo-call not allowed for this user, please ask the admin";
 exit 1
fi

echo " 1. check SPI-interface on this platform!"
echo "   SPI interface must be disabled on this platform !"
echo "     call: sudo raspi-config"
echo "     select: 3 Interface Options"
echo "     select: I4 SPI Enable/disable"
echo "     select: <No> for:Would you like the SPI interface to be enabled?"
echo "     select: <OK>"
echo "     select: <Finish>"
echo
echo -n "   SPI interface disabled  (y/n) ?"
read spi_disabled
echo
if [ "$spi_disabled" = "n" ]; then
  sudo raspi-config
fi

echo " 2. check the heater-bus. must NOT be connected to 'ht_pitiny'"
echo -n "   heater-bus disconnected (y/n) ?"
read bus_disconnected
echo
if [ "$bus_disconnected" = "n" ]; then
 echo " -> disconnect the heater-bus from the board"
 exit 1
fi

echo " 3. check avrdude availability"
which avrdude >/dev/null
if [ $? -eq 0 ]; then
 echo " -> avrdude available";
else
 if [ -f  ./avrdude_install.sh ]; then
   echo "avrdude NOT available -> installation started";
   ./avrdude_install.sh
   echo " -> avrdude installation done"
  else
   echo "avrdude NOT available -> installation required";
   exit 1
 fi
fi

echo "------------------------------------------------------------------------------------"
echo "Load software to 'ht_pitiny-board' (setting fuses, flash and eeprom)"
echo "------------------------------------------------------------------------------------"
if [ -f  ~/HT3/sw/etc/sysconfig/spi_clk_on.py ]; then
  cd ~/HT3/sw/etc/sysconfig/
  sudo ./spi_clk_on.py
fi

cd ${running_path}

echo "  1. Set fuses to values: Ext=F5;High=D5;Low=EE"
sudo avrdude -p ATtiny841 -c linuxgpio -C +RPi_gpio.conf -U lfuse:w:0xEE:m -U hfuse:w:0xD5:m -U efuse:w:0xF5:m

echo "  ----------------------------------------------------------------------------------"
echo "  2. Get latest sw-release from github.com"
# remove old versions
 rm -f ./pitiny.hex
 rm -f ./pitiny.eep
# get current versions
wget https://github.com/norberts1/hometop_ht_transceiver/raw/master/sw/pitiny/pitiny/Release/pitiny.hex
wget https://github.com/norberts1/hometop_ht_transceiver/raw/master/sw/pitiny/pitiny/Release/pitiny.eep

echo "  ----------------------------------------------------------------------------------"
echo "  3. Write programm to flash"
sudo avrdude -p ATtiny841 -c linuxgpio -C +RPi_gpio.conf -v -U flash:w:pitiny.hex

echo "  ----------------------------------------------------------------------------------"
echo "  4. Write data to eeprom"
sudo avrdude -p ATtiny841 -c linuxgpio -C +RPi_gpio.conf -v -U eeprom:w:pitiny.eep

if [ -f  ~/HT3/sw/etc/sysconfig/spi_clk_off.py ]; then
  cd ~/HT3/sw/etc/sysconfig/
  sudo ./spi_clk_off.py
fi

echo "preparing of 'ht_pitiny-board' done"
echo "------------------------------------------------------------------------------------"

