.subckt current_mirror_ota id vinn vinp vss vdd voutp vbiasnd
m17 net16 vinn net24 vss nmos_rvt w=27e-9 l=20e-9 nfin=28
m16 net24 id vss vss nmos_rvt w=27e-9 l=20e-9 nfin=10
m15 net27 vinp net24 vss nmos_rvt w=27e-9 l=20e-9 nfin=28
m14 id id vss vss nmos_rvt w=27e-9 l=20e-9 nfin=10
m11 vbiasnd vbiasnd vss vss nmos_rvt w=27e-9 l=20e-9 nfin=24
m10 voutp vbiasnd vss vss nmos_rvt w=27e-9 l=20e-9 nfin=24
m21 net16 net16 vdd vdd pmos_rvt w=27e-9 l=20e-9 nfin=60
m20 m20stack net16 vdd vdd pmos_rvt w=27e-9 l=20e-9 nfin=240
m20s vbiasnd net16 m20stack vdd pmos_rvt w=27e-9 l=20e-9 nfin=240
m19 net27 net27 vdd vdd pmos_rvt w=27e-9 l=20e-9 nfin=60
m18 m18stack net27 vdd vdd pmos_rvt w=27e-9 l=20e-9 nfin=240
m18s voutp net27 m18stack vdd pmos_rvt w=27e-9 l=20e-9 nfin=240
.END
