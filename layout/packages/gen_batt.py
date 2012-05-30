#!/usr/bin/python

import gen_footprint as gf

gf.scale/=25.4

w=6.07
h=6.07

oa=30.73
centers=oa-w

gf.start_footprint('BATT-HOLDER')

gf.pad((-centers/2,0),w,h,1)
gf.pad((+centers/2,0),w,h,1)
gf.pad((0,0),w,h,2) # neg

gf.file_silk(file('batt.pcb'),100000,100000)

gf.end_footprint()
