#!/usr/bin/python

import gen_footprint as gf

gf.scale/=25.4

D=20
Q=0.950
R=2.5
S=2.0
T=1.07
U=1.0
V=8.0
W=21.070
RFx=5.5
RFg=40.0

gf.start_footprint('ANT-MODULE')

# seven pads along top
top=W/2
for pad in range(7):
    h=U+S*pad
    v=W/2+T-R/2

    gf.pad((h,v),Q,R,4+pad)
    gf.pad((h,-v),Q,R,17-pad)
    

for pad in range(3):
    v=S*(pad-1)
    h=0
    gf.pad((h,-v),R,Q,3-pad)

gf.p_silk((0,S+Q),(0,V+S))
gf.p_silk((0,-S-Q),(0,-V-S))

gf.p_silk((D,V+S),(D,-V-S))
gf.p_silk((D-RFx,V+S),(D-RFx,-V-S))

gf.p_silk((D,-V-S),(D-RFx,V+S))
gf.p_silk((D-RFx,-V-S),(D,V+S))

gf.p_silk((D,-V-S),(D-RFx,-V-S))
gf.p_silk((D-RFx,V+S),(D,V+S))

gf.end_footprint()
