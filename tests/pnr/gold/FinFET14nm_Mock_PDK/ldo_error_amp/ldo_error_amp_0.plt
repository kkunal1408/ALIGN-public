#Use this file as a script for gnuplot
#(See http://www.gnuplot.info/ for details)

set title" #Blocks= 16, #Terminals= 5, #Nets= 16, Area=3.21485e+08, HPWL= 189872 "

set nokey
#   Uncomment these two lines starting with "set"
#   to save an EPS file for inclusion into a latex document
# set terminal postscript eps color solid 20
# set output "result.eps"

#   Uncomment these two lines starting with "set"
#   to save a PS file for printing
# set terminal postscript portrait color solid 20
# set output "result.ps"


set xrange [-19370:19370]

set yrange [-50:19370]

set label "mn56" at 11200 , 5880 center 

set label "B" at 11200 , 7056


set label "D" at 11040 , 4368


set label "G" at 11040 , 5880


set label "S" at 11200 , 4200


set label "mn40" at 15840 , 5880 center 

set label "B" at 15840 , 7056


set label "G" at 15680 , 5880


set label "S" at 15520 , 4284


set label "mn41" at 12640 , 5880 center 

set label "B" at 12640 , 7056


set label "D" at 12480 , 4368


set label "G" at 12480 , 5880


set label "S" at 12640 , 4200


set label "mn21" at 800 , 5880 center 

set label "B" at 800 , 7056


set label "D" at 640 , 4368


set label "G" at 640 , 5880


set label "S" at 800 , 4200


set label "mn55" at 15520 , 13608 center 

set label "B" at 15520 , 14784


set label "D" at 15520 , 12096


set label "G" at 15520 , 13608


set label "S" at 15520 , 11928


set label "mn3" at 14080 , 9744 center 

set label "B" at 14080 , 10920


set label "G" at 13920 , 9744


set label "S" at 13760 , 8148


set label "mn2" at 11360 , 2016 center 

set label "B" at 11360 , 3192


set label "D" at 11360 , 504


set label "G" at 11360 , 2016


set label "S" at 11360 , 336


set label "mmp5" at 12960 , 2016 center 

set label "B" at 12960 , 3192


set label "D" at 12800 , 1260


set label "S" at 12960 , 336


set label "mmp1" at 15680 , 9744 center 

set label "B" at 15680 , 10920


set label "D" at 15680 , 8232


set label "G" at 15680 , 9744


set label "S" at 15680 , 8064


set label "mmn322" at 14400 , 5880 center 

set label "B" at 14400 , 7056


set label "D" at 14240 , 4368


set label "G" at 14240 , 5880


set label "S" at 14400 , 4200


set label "mmn42" at 13760 , 14784 center 

set label "B" at 13760 , 17136


set label "D" at 13600 , 14028


set label "S" at 13440 , 13104


set label "mn38_mn39_mn37_mn20" at 5360 , 6216 center 

set label "B" at 1600 , 4032


set label "DA" at 1600 , 4032


set label "DB" at 1600 , 4032


set label "GA" at 1600 , 4032


set label "S" at 1600 , 4032


set label "mp34_mp28_mp22" at 2800 , 17136 center 

set label "B" at 160 , 15120


set label "DA" at 160 , 15120


set label "DB" at 160 , 15120


set label "DC" at 160 , 15120


set label "S" at 160 , 15120


set label "mp41_mp4" at 14880 , 2016 center 

set label "B" at 14880 , 3192


set label "DA" at 14560 , 1260


set label "DB" at 14880 , 672


set label "S" at 14880 , 336


set label "mmp33_mmp30_mmp29" at 6480 , 11760 center 

set label "B" at 160 , 8568


set label "DA" at 160 , 8568


set label "DB" at 160 , 8568


set label "DC" at 160 , 8568


set label "SA" at 160 , 8568


set label "SB" at 160 , 8568


set label "SC" at 160 , 8568


set label "mn23_mn22" at 8320 , 2016 center 

set label "B" at 8320 , 3192


set label "DA" at 8160 , 504


set label "DB" at 8480 , 672


set label "GA" at 8160 , 2016


set label "GB" at 8480 , 2184


set label "S" at 8320 , 336


set label "vbias_bf" at 16640 , 5880 center 

set label "vbias_an" at 11360 , 0 center 

set label "vg" at 14880 , 0 center 

set label "vfb" at 0 , 2016 center 

set label "vref" at 16640 , 2184 center 

plot[:][:] '-' with lines linestyle 3, '-' with lines linestyle 7, '-' with lines linestyle 1, '-' with lines linestyle 0

# block mn56 select 0 bsize 4
	10560	4032
	10560	7728
	11840	7728
	11840	4032
	10560	4032

# block mn40 select 0 bsize 4
	15200	4032
	15200	7728
	16480	7728
	16480	4032
	15200	4032

# block mn41 select 0 bsize 4
	12000	4032
	12000	7728
	13280	7728
	13280	4032
	12000	4032

# block mn21 select 0 bsize 4
	160	4032
	160	7728
	1440	7728
	1440	4032
	160	4032

# block mn55 select 0 bsize 4
	14720	11760
	14720	15456
	16320	15456
	16320	11760
	14720	11760

# block mn3 select 0 bsize 4
	13440	7896
	13440	11592
	14720	11592
	14720	7896
	13440	7896

# block mn2 select 0 bsize 4
	10560	168
	10560	3864
	12160	3864
	12160	168
	10560	168

# block mmp5 select 0 bsize 4
	12320	168
	12320	3864
	13600	3864
	13600	168
	12320	168

# block mmp1 select 0 bsize 4
	14880	7896
	14880	11592
	16480	11592
	16480	7896
	14880	7896

# block mmn322 select 0 bsize 4
	13760	4032
	13760	7728
	15040	7728
	15040	4032
	13760	4032

# block mmn42 select 0 bsize 4
	12960	11760
	12960	17808
	14560	17808
	14560	11760
	12960	11760

# block mn38_mn39_mn37_mn20 select 0 bsize 4
	1600	4032
	1600	8400
	9120	8400
	9120	4032
	1600	4032

# block mp34_mp28_mp22 select 0 bsize 4
	160	15120
	160	19152
	5440	19152
	5440	15120
	160	15120

# block mp41_mp4 select 0 bsize 4
	13760	168
	13760	3864
	16000	3864
	16000	168
	13760	168

# block mmp33_mmp30_mmp29 select 0 bsize 4
	160	8568
	160	14952
	12800	14952
	12800	8568
	160	8568

# block mn23_mn22 select 0 bsize 4
	6240	168
	6240	3864
	10400	3864
	10400	168
	6240	168


EOF
	10632	7024
	10632	7088
	11768	7088
	11768	7024
	10632	7024

	10808	4336
	10808	4400
	11272	4400
	11272	4336
	10808	4336

	10808	5848
	10808	5912
	11272	5912
	11272	5848
	10808	5848

	10968	4168
	10968	4232
	11432	4232
	11432	4168
	10968	4168

	15272	7024
	15272	7088
	16408	7088
	16408	7024
	15272	7024

	15448	5848
	15448	5912
	15912	5912
	15912	5848
	15448	5848

	15480	4128
	15480	4440
	15560	4440
	15560	4128
	15480	4128

	12072	7024
	12072	7088
	13208	7088
	13208	7024
	12072	7024

	12248	4336
	12248	4400
	12712	4400
	12712	4336
	12248	4336

	12248	5848
	12248	5912
	12712	5912
	12712	5848
	12248	5848

	12408	4168
	12408	4232
	12872	4232
	12872	4168
	12408	4168

	232	7024
	232	7088
	1368	7088
	1368	7024
	232	7024

	408	4336
	408	4400
	872	4400
	872	4336
	408	4336

	408	5848
	408	5912
	872	5912
	872	5848
	408	5848

	568	4168
	568	4232
	1032	4232
	1032	4168
	568	4168

	14792	14752
	14792	14816
	16248	14816
	16248	14752
	14792	14752

	15288	12064
	15288	12128
	15752	12128
	15752	12064
	15288	12064

	15288	13576
	15288	13640
	15752	13640
	15752	13576
	15288	13576

	15128	11896
	15128	11960
	15912	11960
	15912	11896
	15128	11896

	13512	10888
	13512	10952
	14648	10952
	14648	10888
	13512	10888

	13688	9712
	13688	9776
	14152	9776
	14152	9712
	13688	9712

	13720	7992
	13720	8304
	13800	8304
	13800	7992
	13720	7992

	10632	3160
	10632	3224
	12088	3224
	12088	3160
	10632	3160

	11128	472
	11128	536
	11592	536
	11592	472
	11128	472

	11128	1984
	11128	2048
	11592	2048
	11592	1984
	11128	1984

	10968	304
	10968	368
	11752	368
	11752	304
	10968	304

	12392	3160
	12392	3224
	13528	3224
	13528	3160
	12392	3160

	12760	432
	12760	2088
	12840	2088
	12840	432
	12760	432

	12728	304
	12728	368
	13192	368
	13192	304
	12728	304

	14952	10888
	14952	10952
	16408	10952
	16408	10888
	14952	10888

	15448	8200
	15448	8264
	15912	8264
	15912	8200
	15448	8200

	15448	9712
	15448	9776
	15912	9776
	15912	9712
	15448	9712

	15288	8032
	15288	8096
	16072	8096
	16072	8032
	15288	8032

	13832	7024
	13832	7088
	14968	7088
	14968	7024
	13832	7024

	14008	4336
	14008	4400
	14472	4400
	14472	4336
	14008	4336

	14008	5848
	14008	5912
	14472	5912
	14472	5848
	14008	5848

	14168	4168
	14168	4232
	14632	4232
	14632	4168
	14168	4168

	13032	17104
	13032	17168
	14488	17168
	14488	17104
	13032	17104

	13560	12024
	13560	16032
	13640	16032
	13640	12024
	13560	12024

	13400	11856
	13400	14352
	13480	14352
	13480	11856
	13400	11856

	1600	4032
	1600	4032
	1600	4032
	1600	4032
	1600	4032

	1600	4032
	1600	4032
	1600	4032
	1600	4032
	1600	4032

	1600	4032
	1600	4032
	1600	4032
	1600	4032
	1600	4032

	1600	4032
	1600	4032
	1600	4032
	1600	4032
	1600	4032

	1600	4032
	1600	4032
	1600	4032
	1600	4032
	1600	4032

	160	15120
	160	15120
	160	15120
	160	15120
	160	15120

	160	15120
	160	15120
	160	15120
	160	15120
	160	15120

	160	15120
	160	15120
	160	15120
	160	15120
	160	15120

	160	15120
	160	15120
	160	15120
	160	15120
	160	15120

	160	15120
	160	15120
	160	15120
	160	15120
	160	15120

	13832	3160
	13832	3224
	15928	3224
	15928	3160
	13832	3160

	14520	432
	14520	2088
	14600	2088
	14600	432
	14520	432

	14648	640
	14648	704
	15112	704
	15112	640
	14648	640

	14168	304
	14168	368
	15592	368
	15592	304
	14168	304

	160	8568
	160	8568
	160	8568
	160	8568
	160	8568

	160	8568
	160	8568
	160	8568
	160	8568
	160	8568

	160	8568
	160	8568
	160	8568
	160	8568
	160	8568

	160	8568
	160	8568
	160	8568
	160	8568
	160	8568

	160	8568
	160	8568
	160	8568
	160	8568
	160	8568

	160	8568
	160	8568
	160	8568
	160	8568
	160	8568

	160	8568
	160	8568
	160	8568
	160	8568
	160	8568

	6312	3160
	6312	3224
	10328	3224
	10328	3160
	6312	3160

	6808	472
	6808	536
	9512	536
	9512	472
	6808	472

	7128	640
	7128	704
	9832	704
	9832	640
	7128	640

	6808	1984
	6808	2048
	9512	2048
	9512	1984
	6808	1984

	7128	2152
	7128	2216
	9832	2216
	9832	2152
	7128	2152

	6648	304
	6648	368
	9992	368
	9992	304
	6648	304


EOF

	16620	5860
	16620	5900
	16660	5900
	16660	5860
	16620	5860

	11340	-20
	11340	20
	11380	20
	11380	-20
	11340	-20

	14860	-20
	14860	20
	14900	20
	14900	-20
	14860	-20

	-20	1996
	-20	2036
	20	2036
	20	1996
	-20	1996

	16620	2164
	16620	2204
	16660	2204
	16660	2164
	16620	2164

EOF

#Net: vbias6
	11040	4368
	14560	1260
	11040	4368


#Net: vbias_bf
	11040	5880
	15680	5880
	11040	5880

	11040	5880
	15520	13608
	11040	5880

	11040	5880
	16640	5880
	11040	5880


#Net: vbias4
	12480	4368
	13440	13104
	12480	4368


#Net: vbias_an
	12480	5880
	13920	9744
	12480	5880

	12480	5880
	11360	2016
	12480	5880

	12480	5880
	11360	0
	12480	5880


#Net: vbias2
	640	4368
	160	8568
	640	4368


#Net: vbias1
	640	5880
	160	15120
	640	5880

	640	5880
	160	8568
	640	5880


#Net: vbias3
	800	4200
	13600	14028
	800	4200

	800	4200
	1600	4032
	800	4200


#Net: v3_d
	15520	12096
	15680	8232
	15520	12096

	15520	12096
	14240	5880
	15520	12096


#Net: vcom1
	11360	504
	8320	336
	11360	504


#Net: vg
	12800	1260
	15680	8064
	12800	1260

	12800	1260
	14240	4368
	12800	1260

	12800	1260
	14880	672
	12800	1260

	12800	1260
	14880	0
	12800	1260


#Net: v3
	15680	9744
	1600	4032
	15680	9744

	15680	9744
	160	8568
	15680	9744


#Net: v4
	1600	4032
	160	8568
	1600	4032


#Net: v2
	160	15120
	160	8568
	160	15120

	160	15120
	8160	504
	160	15120


#Net: v1
	160	15120
	160	8568
	160	15120

	160	15120
	8480	672
	160	15120


#Net: vfb
	8160	2016
	0	2016
	8160	2016


#Net: vref
	8480	2184
	16640	2184
	8480	2184


EOF

pause -1 'Press any key'