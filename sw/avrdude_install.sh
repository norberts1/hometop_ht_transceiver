#! /bin/bash
#
 ##
 # #################################################################
 ## Copyright (c) 2023 Norbert S. <junky-zs at gmx dot de>
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
 # Ver:0.1    /Datum 04.03.2023
 #################################################################
#
# see url:
#  https://github.com/avrdudes/avrdude/wiki/Building-AVRDUDE-for-Linux
#
echo "------------------------------------------------------------------------------------"
echo "check avrdude availability"
which avrdude >/dev/null
if [ $? -eq 0 ]; then
 echo " -> avrdude already available";
 exit
fi

echo "avrdude NOT available -> installation started";

cd
sudo apt-get update
sudo apt-get -y upgrade
echo "------------------------------------------------------------------------------------"
echo "1. Get and install required software"
sudo apt-get -y install build-essential git cmake flex bison libelf-dev libusb-dev libhidapi-dev libftdi1-dev libreadline-dev
git clone https://github.com/avrdudes/avrdude.git
cd ~/avrdude/
echo "------------------------------------------------------------------------------------"
echo "2. Build and install 'avrdude'"
./build.sh 
cd ~/avrdude/
sudo cmake --build build_linux --target install
