#!/usr/bin/ruby
#
#
require 'rim'

width=640
height=480
bgsbt_threshold=20
area_min_threshold=1000

cam=Rim::open_camera(1,width,height,15.0)

previous=cam.capture.to_gray.to_i
previous.show
sleep(5)
loop{
    current=cam.capture.to_gray.to_i

    previous=previous.sbt!(current).abs
    label=previous.binarize(bgsbt_threshold
                            ).to_byte.labeling(area_min_threshold)
    if(label.labelinfo.size>0)
        mask=label.eq(1)
        r=current[]
        r[mask]=255
        g=current[]
        g[mask]=0
        b=current[]
        b[mask]=255
        b.add_plane([g,r]).show
    else
        current.show
    end

    previous=current[]
    sleep(5)
}
