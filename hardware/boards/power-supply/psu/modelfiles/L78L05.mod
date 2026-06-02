* L78L05 SPICE Behavioral Macro-Model
* Pins: 1:INPUT  2:GND  3:OUTPUT
.SUBCKT L78L05 1 2 3
Xmod_reg1 1 2 3_int LM7805_Behave
R_out 3_int 3 0.1
.ENDS

.SUBCKT LM7805_Behave 1 2 3
V_Ref 4 2 DC 5.0
E_Reg 3 2 VALUE = { V(1,2) > 7.0 ? 5.0 : V(1,2)-2.0 }
R_in 1 2 1Meg
.ENDS