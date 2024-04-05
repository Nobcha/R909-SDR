# R909-DSP
R909-DSP is a radio for the airband and the FM broadcasting. 
It is configured with Si5351a, Si4732, and TA2003 mainly.
TA2003 and Si5351a are sharing the first mixer and the local oscillator.
Si4732 is acting as a mother 21.4MHz receiver for the airband and the FM radio itself.
According with PU2CLR's advice I adopted PU2CLR Si4735/32 library for this sketch.
There are 2 kinds of display version, as the one for SSD and the other for WE1602A. 

Operation is depending on the rotary encoder and the rotary encoder's push switch.
There are 3 major modes, as FUNC, parameter selection at every function mode, and to memory the parameter into EEPROM.
FUNC mode is to select every function. You shall select the function by rotating and determine the one by single pushing.
In every function mode you can change the parameter by rotating and store the parameter on to EEPROM bt double pushing.
To escape out from every function mode to push rortary encoder switch single.
Rotating, one pushing, and two pushing is the identical operation for this sketch.

There are two kinds of PCBs as Panel and RF to compose the radio. They are connecting by pinhedder connector. 
The ATmega328P with the Arduino boot loader, the switches, and the display are locating on the Panel PCB.

I resumed the uploaded files list as "r909-DSP_list_1.jpg" and "r909-DSP_list_1.jpg".

I introduced how to operate on YOUTUBE as below:
1602A debugging "https://www.youtube.com/watch?v=wb-YdMU-vz0"
OLED panel "https://www.youtube.com/watch?v=UpdDh7BYJYI" "https://www.youtube.com/shorts/03Hpvuk5BYI"

If you use my Gerber files or information to make PCB, please resister PCBGOGO via "https://www.pcbgogo.jp/promo/nobcha23"

I'm running BLOG and please visit "https://nobcha23.hatenadiary.com/"

 nobcha
