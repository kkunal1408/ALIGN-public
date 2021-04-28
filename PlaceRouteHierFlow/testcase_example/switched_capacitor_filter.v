//Verilog block level netlist file for switched_capacitor_filter
//Generated by UMN for ALIGN project 


module switched_capacitor_combination ( Vin, agnd, Vin_ota, Voutn, phi1, phi2 ); 
input Vin, agnd, Vin_ota, Voutn, phi1, phi2;

Cap_60fF c0 ( .MINUS(net63), .PLUS(net72) ); 
Switch_NMOS_n12_X1_Y1 m4 ( .D(net72), .G(phi2), .S(agnd) ); 
Switch_NMOS_n12_X1_Y1 m6 ( .D(net72), .G(phi1), .S(Vin) ); 
DP_NMOS_n12_X1_Y1 m7_m5 ( .DA(Vin_ota), .GA(phi1), .S(net63), .DB(agnd), .GB(phi2) ); 
DP_NMOS_n12_X1_Y1 m0_m3 ( .DA(Voutn), .GA(phi1), .S(net67), .DB(agnd), .GB(phi2) ); 

Cap_30fF_Cap_60fF c1_c3 ( .MINUS1(net67), .PLUS1(net63), .MINUS2(Voutn), .PLUS2(Vin_ota) );
endmodule

module switched_capacitor_filter ( vinp, voutn, vss, vinn, agnd, voutp, id ); 
input vinp, voutn, vss, vinn, agnd, voutp, id;

telescopic_ota xi0 ( .d1(id), .vbiasn(vbiasn), .vbiasnd(vbiasnd), .vbiasp1(vbiasp1), .vbiasp2(vbiasp2), .vdd(vdd), .vinn(net64), .vinp(net66), .voutn(voutn), .voutp(voutp), .vss(vss) ); 
switched_capacitor_combination m6_c0_m4_m3_m5_c1_m7_c3_m0 ( .Vin(vinn), .agnd(agnd), .Vin_ota(net66), .Voutn(voutn), .phi1(phi1), .phi2(phi2) ); 
switched_capacitor_combination m12_c4_m8_m11_m9_c7_m10_c6_m14 ( .Vin(vinp), .agnd(agnd), .Vin_ota(net64), .Voutn(voutp), .phi1(phi1), .phi2(phi2) ); 

Cap_30fF_Cap_30fF c2_c5 ( .MINUS1(net66), .PLUS1(vinp), .MINUS2(net64), .PLUS2(vinn) );
Cap_60fF_Cap_60fF c8_c9 ( .MINUS1(vss), .PLUS1(voutn), .MINUS2(vss), .PLUS2(voutp) );
endmodule

module telescopic_ota ( d1, vbiasn, vbiasnd, vbiasp1, vbiasp2, vdd, vinn, vinp, voutn, voutp, vss ); 
input d1, vbiasn, vbiasnd, vbiasp1, vbiasp2, vdd, vinn, vinp, voutn, voutp, vss;

CMC_PMOS_S_n12_X1_Y1 m2_m1 ( .DA(net012), .G(vbiasp1), .DB(net06), .S(vdd) ); 
DP_NMOS_n12_X3_Y2 m3_m0 ( .DA(net014), .GA(vinn), .S(net10), .DB(net8), .GB(vinp) ); 
CMFB_NMOS_n12_X3_Y1 m5_m4 ( .DA(d1), .S(vss), .DB(net10), .GB(vbiasnd) ); 
CMC_PMOS_n12_X2_Y1 m6_m7 ( .DA(voutn), .G(vbiasp2), .DB(voutp), .SA(net06), .SB(net012) ); 
CMC_NMOS_n12_X3_Y1 m9_m8 ( .DA(voutn), .G(vbiasn), .DB(voutp), .SA(net8), .SB(net014) ); 

endmodule