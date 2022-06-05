**Induction balance metal detector**

This project is an Induction Balance Metal Detector built around Arduino Nano and a 12864 display module with encoder and piezo buzzer of the sort intended for use with 3D printers. The search coil comprises two D-shaped coils fastened on opposite side of a 250mm diameter plastic plate. The coils are wrapped in grounded aluminium foil (with a gap at one point so that it doesn't behave like a shorted turn) so that capacitance between the coils and the ground doesn't cause the system to go out of balance.

The number of turns of wire in each coil must give an inductance of approx. 4.1mH so that the resonant frequency of each coil in conjunction with the 100nF tuning capacitors is 7.8125kHz. I used about 100 turns. The tuning capacitors can be adjusted if necessary. Fine adjustment can be made to the frequency in the firmware.
