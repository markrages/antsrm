#!/usr/bin/python


scale=100000

clear=0.008*scale

def start_footprint(name):
    global fd
    fd=file(name,'w')
    print >>fd, 'Element["" "" "" "" 1000 1000 1000 1000 0 100 ""]\n('
    

def p_silk(start, end):
    sx,sy=start
    ex,ey=end
    print >>fd, 'ElementLine[%d %d %d %d 1000]'%(sx*scale,sy*scale,ex*scale,ey*scale)
    
    return (ex,ey)

def file_silk(in_fd,offset_x,offset_y):
    for l in in_fd.readlines():
        l=l.strip()
        if l.startswith("Line"):
            line=l.split('Line[')[1]
            line=line.rstrip("]")
            x1,y1,x2,y2=[int(x) for x in line.split()[:4]]
            x1-=offset_x
            x2-=offset_x
            y1-=offset_y
            y2-=offset_y
            p_silk((x1/scale,y1/scale),(x2/scale,y2/scale))
            

def silk_poly(*edges):
    for s,e in zip(edges,(edges[-1],)+edges[:-1]):        
        p_silk(s,e)

import math    
def silk_filled_tri(*edges):
    centroid=[sum(x)/float(len(x)) for x in zip(*edges)]

    distances=[(math.sqrt(sum([(a-b)**2 for a,b in zip(point,centroid)]))) for point in edges]
    
    print distances

    iterations=int(max(distances) / (1000 / scale))

    for d in range(iterations+1):
        xscale=float(d)/iterations
        print xscale
        points=[[c+(p-c)*xscale for c,p in zip(centroid,point)] for point in edges]
        
        silk_poly(*points)



def silk_boxer(center, size):
    cx,cy=center
    sw,sh=size
    p_silk((cx-sw/2,cy-sh/2),(cx+sw/2,cy-sh/2))
    p_silk((cx+sw/2,cy-sh/2),(cx+sw/2,cy+sh/2))
    p_silk((cx+sw/2,cy+sh/2),(cx-sw/2,cy+sh/2))
    p_silk((cx-sw/2,cy+sh/2),(cx-sw/2,cy-sh/2))
    
def silk_circle(center, dia, start=0, deg=360):
    cx,cy=center
    print >>fd, 'ElementArc[%d %d %d %d %d %d 1000]'%(cx*scale,cy*scale,0.5*dia*scale,0.5*dia*scale,start,deg)

def hole(center, dia):
    cx,cy=center
    print >>fd, 'Pin[%d %d 0 0 %d %d "" "" "hole" ]'%(cx*scale,cy*scale,dia*scale,dia*scale)
    
def pin(center, dia, pinnumber, flags=None, annulus=None):
    if flags==None: flags=[]

    flags=["pin"]+flags
    if annulus==None:
        annulus=dia*1.2
    shrink=0.005
    cx,cy=center

    args=(cx*scale,
          cy*scale,
          (dia+annulus)*scale,
          0.020*100000,
          (dia+annulus)*scale,
          dia*scale,
          "pin %d"%pinnumber,
          "%d"%pinnumber,",".join(flags))

    print >>fd, 'Pin[%d %d %d %d %d %d "%s" "%s" "%s" ]'%args

def pad(center, width, height, pinnumber, flags=None):
    if flags==None: flags=['square']

    shrink=0.005
    cx,cy=center

    if width>height: # horizontal
        start=(cx-width/2+height/2,cy)
        end=(cx+width/2-height/2,cy)
        thick=height
    else: # vertical
        start=(cx, cy-width/2+height/2)
        end=(cx, cy+width/2-height/2)
        thick=width
        
    #print "unscaled ",start,end,width,height

    start=(scale*start[0],scale*start[1])
    end=(scale*end[0],scale*end[1])

    #print "scaled ",start,end

    args=start + end + \
        (thick*scale, 
         clear*2,
         clear+thick*scale, # mask
         "pin %d"%pinnumber,
         "%d"%pinnumber,",".join(flags))

    print >>fd, 'Pad[%d %d %d %d %d %d %d "%s" "%s" "%s" ]'%args

def pad_corners(corner_1,corner_2,pinnumber,flags=None):

    centroid=[sum(x)/float(len(x)) for x in zip(corner_1,corner_2)]
    x1,y1=corner_1
    x2,y2=corner_2

    width=abs(x1-x2)
    height=abs(y1-y2)

    pad(centroid, width, height, pinnumber, flags)

def end_footprint():
    print >>fd, ')'
    
    
    
    
 
