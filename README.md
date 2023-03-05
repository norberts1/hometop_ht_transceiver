# hometop_ht_transceiver

HW- and SW-parts to receive/transmit seriell data from/to heating-system.

**Table of Contents**

- [Introduction](#intro)
- [Used Hardware](#hardware)
- [Software](#software)
- [Software Installation](#softwareinstallation)
- [Documentation](#docu)
- [License](#license)


## Introduction<a name="intro"></a>

This hardware is designed for serial data-communication between 'heater-bus' and external controlling software.  

Currently only heater-systems from german manufacturer: **`Junkers/Bosch`** and system-bus: **`Heatronic/EMS2 (c)`** are supported. 

## Used Hardware<a name="hardware"></a>

There are two active boards (alias name: ht_transceiver) created for the RaspberryPi(c).  
They differs only in hardware-design and used uC's but have the identical functionality.  

The components on the 'ht_piduino-board' are 'MTH' (Mounted Through Hole) and the 'ht_pitiny-board' has SMD (Surface Mounted Devices) components.  
The 'ht_pitiny-board' is a bit more tiny and is using less space on the RaspberryPi.

The table shows the currently available active boards: 
<br> 

<table>
<tr>
    <th>Board-name </th>
    <th>function</th>
    <th>Comment</th>
    <th>Interface</th>
</tr>
<tr>
    <td>ht_pitiny</td>
    <td>transmit- and receiving Bus - data</td>
    <td>active ht_transceiver-board for RPi<br>using ATtiny841.</td>
    <td>serial asynchronous data.<br> 9600Baud to/from heater-bus.<br>19200Baud to/from RPi.</td>
</tr>
<tr>
    <td>ht_piduino</td>
    <td>transmit- and receiving Bus - data</td>
    <td>active ht_transceiver-board for RPi<br>using ATMega328.</td>
    <td>serial asynchronous data.<br> 9600Baud to/from heater-bus.<br>19200Baud to/from RPi.</td>
</tr>
</table>

- Modul: ht_pitiny

![Modul: ht_pitiny](https://github.com/norberts1/hometop_HT3/blob/master/HT3/docu/pictures/HT_pitiny.png)
<br> 

- Modul: ht_piduino

![Modul: ht_piduino](https://github.com/norberts1/hometop_HT3/blob/master/HT3/docu/pictures/HT_piduino.png)
<br> 

For the 'ht_transceiver'- hardware see gerber-files and documentation (folder: **`~/hometop_ht_transceiver/hw`** )   
and project: [hometop_ht](https://github.com/norberts1/hometop_HT3).

## Software<a name="software"></a>

The **software** is written in **c and cpp** and designed for receiving and transmitting heater-busdata with following features: 
<br>
<table>
<tr>
    <th>SW-Part </th>
    <th>function</th>
</tr>
<tr>
    <td>receive heater-busdata</td>
    <td>Receiving heater-bus data.<br>Break-Signal detection.<br>Sending received message attached with header to external decoding-SW.</td>
</tr>
<tr>
    <td>receive external commands for heater-bus</td>
    <td>Receiving commands for the heater-bus.<br>Break-Signal generation.<br>Sending generated command-block to heater-bus.</td>
</tr>
<tr>
    <td>receive external commands for board-setup</td>
    <td>Receiving commands for ht_transceiver-board setup.<br>Own Device ID change.<br>Reset of ht_transceiver.</td>
</tr>
<tr>
    <td>Modul device ID handling</td>
    <td>Own Device ID detection.<br>Generating polling-answers.<br>Sending polling-answers to heater-bus.</td>
</tr>
</table>


## Software Installation<a name="softwareinstallation"></a>

### Preconditions:

There are some preconditions to install or update the software on any ht_transceiver-board.

- Board must be checkt, mainly that external CPU-crystal must be workable.  
- Board has to be mounted on RaspberryPi.  
- Any heater-bus connection must be disconnected. 
- 'avrdude' **must not be** installed manually with 'apt install avrdude', cause the default one has no lib: 'linuxgpio' available.
- 'sudo' must be enabled for the user.
- That interface: 'SPI' on RaspberryPi has to be 'disabled'. This will be forced by the installation-script.

### Installation / Update

Step into folder:   
 cd hometop_ht_transceiver/sw  

Call the installaton-script for:

<table>
<tr>
    <th>Board-name </th>
    <th>script-name</th>
    <th>function</th>
</tr>
<tr>
    <td>ht_pitiny</td>
    <td>prepare_pitiny_board.sh</td>
    <td>1.check platform.<br>2.check SPI-IF.<br>3.Install and compile avrdude if not already done.<br>4.Setup fuses.<br>5.Flash latest sw-release.<br>6.Write eeprom-data.</td>
</tr>
<tr>
    <td>ht_piduino</td>
    <td>TBD</td>
    <td></td>
</tr>
</table>  

### Postconditions:

- Check the output on the installation-terminal for any errors.  
- Check the green flashing LED on the 'ht_transceiver-board'. If no flashing LED is visable, some errors had occurred.
- Switch off RaspberryPi and reconnect the ht_transceiver-board' to the heater-bus.

## Documentation<a name="docu"></a>

For 'ht_transceiver'-description see the documentation (folder: **`~/hometop_ht_transceiver/docu`** ).


## License<a name="license"></a>
> Hardware:cc by-nc-sa 4.0  ([details](https://creativecommons.org/licenses/by-nc-sa/4.0/))

