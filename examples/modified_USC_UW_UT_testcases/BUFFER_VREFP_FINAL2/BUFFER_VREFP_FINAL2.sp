.subckt BUFFER_VREFP_FINAL2 gnd ibias sw<2> sw<1> sw<0> vdd vref vrefp
m58 vrefp net459 net0129 vdd lvtpfet w=w0 l=l0
m43 net0125 net431 vrefp vdd lvtpfet w=w1 l=l0
m15 vfb net450 net0115 vdd lvtpfet w=w2 l=l0
m28 vrefp net459 net0127 vdd lvtpfet w=w3 l=l0
m27 net431 net431 vfb vdd lvtpfet w=w4 l=l0
m29 net459 net431 vrefp vdd lvtpfet w=w5 l=l0
m106 vdd net431 vdd vdd lvtpfet w=w6 l=l1
m32 net0132 net431 vrefp vdd lvtpfet w=w5 l=l0
m33 vrefp net459 net0113 vdd lvtpfet w=w7 l=l0
m34 vrefp net459 net0135 vdd lvtpfet w=w8 l=l0
m35 net0134 net431 vrefp vdd lvtpfet w=w9 l=l0
m62 swn2 swp2 gnd gnd nfet w=w10 l=l2
m59 swp2 sw<2> gnd gnd nfet w=w10 l=l2
m57 net0125 ibias net468 gnd nfet w=w11 l=l0
m55 net468 swn2 gnd gnd nfet w=w12 l=l2
m54 swn1 swp1 gnd gnd nfet w=w10 l=l2
m51 swp1 sw<1> gnd gnd nfet w=w10 l=l2
m50 swn0 swp0 gnd gnd nfet w=w10 l=l2
m48 swp0 sw<0> gnd gnd nfet w=w10 l=l2
m20 net469 swn1 gnd gnd nfet w=w13 l=l2
m19 net470 swn0 gnd gnd nfet w=w13 l=l2
m18 net463 vdd gnd gnd nfet w=w14 l=l2
m17 net427 vdd gnd gnd nfet w=w15 l=l2
m16 net462 vdd gnd gnd nfet w=w15 l=l2
m9 net466 vdd gnd gnd nfet w=w15 l=l2
m7 net467 vdd gnd gnd nfet w=w15 l=l2
m2 net465 vdd gnd gnd nfet w=w15 l=l2
m0 net464 vdd gnd gnd nfet w=w16 l=l2
m6 net450 net412 net462 gnd nfet w=w17 l=l2
m1 net412 net412 net467 gnd nfet w=w17 l=l2
m3 net418 ibias net466 gnd nfet w=w18 l=l0
m4 ibias ibias net464 gnd nfet w=w19 l=l0
m5 net411 ibias net465 gnd nfet w=w19 l=l0
m8 net423 vfb net418 gnd nfet w=w20 l=l0
m10 net412 vfb net418 gnd nfet w=w20 l=l0
m11 net417 vref net418 gnd nfet w=w20 l=l0
m12 net450 vref net418 gnd nfet w=w20 l=l0
m21 net431 ibias net427 net427 nfet w=w19 l=l0
m30 net459 ibias net463 gnd nfet w=w21 l=l0
m31 net0132 ibias net470 gnd nfet w=w21 l=l0
m36 net0134 ibias net469 gnd nfet w=w22 l=l0
m61 swn2 swp2 vdd vdd pfet w=w23 l=l2
m60 swp2 sw<2> vdd vdd pfet w=w23 l=l2
m53 swn1 swp1 vdd vdd pfet w=w23 l=l2
m52 swp1 sw<1> vdd vdd pfet w=w23 l=l2
m49 swn0 swp0 vdd vdd pfet w=w23 l=l2
m47 swp0 sw<0> vdd vdd pfet w=w23 l=l2
m46 net0135 swp1 vdd vdd pfet w=w24 l=l2
m45 net0113 swp0 vdd vdd pfet w=w25 l=l2
m44 net0127 gnd vdd vdd pfet w=w25 l=l2
m56 net0129 swp2 vdd vdd pfet w=w26 l=l2
m42 net0115 gnd vdd vdd pfet w=w27 l=l2
m41 net0124 gnd vdd vdd pfet w=w27 l=l2
m40 net0119 gnd vdd vdd pfet w=w27 l=l2
m39 net0114 gnd vdd vdd pfet w=w27 l=l2
m38 net0128 gnd vdd vdd pfet w=w27 l=l2
m37 net0121 gnd vdd vdd pfet w=w13 l=l2
m13 net450 net411 net0137 vdd pfet w=w28 l=l0
m14 net412 net411 net0122 vdd pfet w=w28 l=l0
m22 net411 net411 net0121 vdd pfet w=w17 l=l0
m23 net423 net423 net0114 vdd pfet w=w29 l=l0
m24 net0137 net423 net0124 vdd pfet w=w11 l=l0
m25 net417 net417 net0119 vdd pfet w=w29 l=l0
m26 net0122 net417 net0128 vdd pfet w=w11 l=l0
.ends BUFFER_VREFP_FINAL2
