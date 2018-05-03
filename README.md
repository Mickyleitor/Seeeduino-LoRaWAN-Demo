# PervasiveSystem2018_LoRaWAN
Hands-On Example to demostrate the technology behind LoRaWAN with Seeeduino LoRa board

## Visual representation
![Visual representation](https://raw.githubusercontent.com/Mickyleitor/PervasiveSystem2018_LoRaWAN/master/Docs/Visual-representation.png)

## Hands-On 
* Plug-in two boards (Make sure they are Seeeduino LoRa board)
* Load the code into any Arduino-IDE compatible
* Change the myID value to whatever you need, in this case having 2 boards we will set up '0' and '1'.
* Load the sketch inside the boards (Remember change the myID !!)
* Once sketch loaded, open COMX monitors (Putty or gtkterm) communicating with two boards.
* Send commands trough master board (Address '0' - Dec 48).
* Format of the commands [cmd:ADDRESS@COMMAND]\
Where ADDRESS is the physical address of the board (the one you set before). 1 character length\
And COMMAND is the type of command you will request to the other board. 1 character length

## Available commands
* 'a' : Get analogical value of A0
* 'b' : Get RSSI from both devices
* 'c' : Get GPS based latitude position
* 'd' : Get GPS based longitude position

## Technical Diagram of the Finite state machine
![Finite-State machine](https://raw.githubusercontent.com/Mickyleitor/PervasiveSystem2018_LoRaWAN/master/Docs/State-Machine.png)
