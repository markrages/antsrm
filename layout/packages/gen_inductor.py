#!/usr/bin/python

import gen_footprint as gf

gf.scale/=25.4

gf.start_footprint('INDUCTOR-SPECIAL')

d=8 # spec sheet says 14, I measure ~7.7mm dia

#gf.hole((0,0),d)

gf.pin((0.075*25.4,0.050*25.4),1.0,1)
gf.pin((-0.225*25.4,0.325*25.4),1.0,1)


#gf.pad((5.5,2.7),3,3,1,[])


#gf.pad((5.5,-2.7),3,3,2,[])





gf.end_footprint()
