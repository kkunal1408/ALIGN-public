************************************************************************
* auCdl Netlist:
* 
* Library Name:  OTA_class
* Top Cell Name: single_ended_low_voltage_cascode_pmos
* View Name:     schematic
* Netlisted on:  Sep 11 21:08:22 2019
************************************************************************

*.BIPOLAR
*.RESI = 2000 
*.RESVAL
*.CAPVAL
*.DIOPERI
*.DIOAREA
*.EQUATION
*.SCALE METER
*.MEGA
.PARAM

*.GLOBAL vdd!
+        gnd!

*.PIN vdd!
*+    gnd!

************************************************************************
* Library Name: OTA_class
* Cell Name:    single_ended_low_voltage_cascode_pmos
* View Name:    schematic
************************************************************************

.SUBCKT single_ended_low_voltage_cascode_pmos Vbiasp Vinn Vinp Voutp
*.PININFO Vbiasp:I Vinn:I Vinp:I Voutp:O
MM7 Voutp Vinn net11 net18 pmos w=WA l=LA nfin=nA
MM6 net13 Vinp net11 net18 pmos w=WA l=LA nfin=nA
MM5 net11 Vbiasp vdd! vdd! pmos w=WA l=LA nfin=nA
MM1 Voutp net17 net20 gnd! nmos w=WA l=LA nfin=nA
MM0 net13 net17 net19 gnd! nmos w=WA l=LA nfin=nA
MM9 net20 net13 gnd! gnd! nmos w=WA l=LA nfin=nA
MM8 net19 net13 gnd! gnd! nmos w=WA l=LA nfin=nA
.ENDS


.SUBCKT LG_nmos_l1 Biasn Vbiasn Vbiasp
*.PININFO Biasn:I Vbiasn:O Vbiasp:O
MM2 Vbiasn Vbiasn gnd! gnd! nmos w=WA l=LA nfin=nA
MM10 Vbiasp Biasn gnd! gnd! nmos w=WA l=LA nfin=nA
MM1 Vbiasn Vbiasp vdd! vdd! pmos w=WA l=LA nfin=nA
MM0 Vbiasp Vbiasp vdd! vdd! pmos w=WA l=LA nfin=nA
.ENDS

.SUBCKT CR1_1 Vbiasn
*.PININFO Vbiasn:O
RRF vdd! Vbiasn res=rK
MM0 Vbiasn Vbiasn gnd! gnd! nmos w=WA l=LA nfin=nA
.ENDS


xiota LG_Vbiasp Vinn Vinp Voutp single_ended_low_voltage_cascode_pmos
xiLG_nmos_l1 Biasn LG_Vbiasn LG_Vbiasp LG_nmos_l1
xibCR1_1 Biasn CR1_1
.END