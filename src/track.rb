#!/usr/bin/ruby
#
#
require 'rim'
require 'mr999'
require 'rim/rim_demo'

BASIC_WIDTH=640.0
BASIC_HEIGHT=480.0

MAGNIFY=1.0

H_RANGE=(0.75..1.0)
L_RANGE=(0.2..0.8)
S_RANGE=(0.35..1.0) #(0.5..1.0)
MIN_AREA=512*MAGNIFY


CONVERGE=30.0*MAGNIFY
CONVERGE_DEG=30.0
TICS=200.0/MAGNIFY


def pick_r(img,h_range,l_range,s_range,min_area)
    buf=img.cv_bgr2hls.to_f
    
    h=NMath::cos(buf[0,false].mul!(NMath::PI).div!(90.0))
    l=buf[1,false].div!(255.0)
    s=buf[2,false].div!(255.0)

    mask=NArray.byte(img.width,img.height).fill!(1)

    mask=mask.and(h.ge(h_range.begin)).and(h.le(h_range.end)) if h_range
    mask=mask.and(l.ge(l_range.begin)).and(l.le(l_range.end)) if l_range
    mask=mask.and(s.ge(s_range.begin)).and(s.le(s_range.end)) if s_range
    
    buf=Rim::Image.new(img.width,img.height,1)
    buf[0,false]=mask
    buf.mul!(255)

    label=buf.labeling(min_area)
    #label[label.ne(1)]=0
    #label.mul!(255)

    label
end

def sleep_to_stop(i,j)
    sleep(i.abs/TICS)
    j.stop(true)
end


mr999=MR999.new

width=BASIC_WIDTH*MAGNIFY
height=BASIC_HEIGHT*MAGNIFY

center=Rim::Point2D.new((width-1.0)/2.0,(height-1.0)/2.0)

cam=Rim::open_camera(1,width.to_i,height.to_i,15.0)

gray=Rim::Image.new(cam.width,cam.height,1)

Signal.trap(:INT){
    mr999.stop
    exit
 }

loop{
    cam.capture.show

    label=pick_r(cam,H_RANGE,L_RANGE,S_RANGE,MIN_AREA)

    if(label.labelinfo.size>0)
        
        #
        # get angle
        #
        #gray.fill!(0)
        #gray[label.eq(1)]=255
        #rect=gray.cv_min_area_rect
        #deg=rect.angle
        #deg-=90 if(rect.height>rect.width)
        deg=0.0

        #
        # get center of image
        #
        c=label.labelinfo[0].rectangle.center
        
        #
        # draw PoI 
        #
        ci=c.to_i
        cam[[0,2],(ci[0]-5)..(ci[0]+5),(ci[1]-5)..(ci[1]+5)]=0
        cam[1,(ci[0]-5)..(ci[0]+5),(ci[1]-5)..(ci[1]+5)]=255
        cam.show

        #
        # move each joint
        #
        delta=c-center
        t1=if(delta[0].abs<CONVERGE)
               mr999.waist.stop(true)
               nil
           elsif(delta[0]<-CONVERGE)
               mr999.waist.reverse(true)
               Thread.start(delta[0],mr999.waist){|i,j| sleep_to_stop(i,j) }
           else
               mr999.waist.normal(true)
               Thread.start(delta[0],mr999.waist){|i,j| sleep_to_stop(i,j) }
           end

        t2=if(delta[1].abs<CONVERGE)
               mr999.elbow.stop(true)
               nil
           elsif(delta[1]<-CONVERGE)
               mr999.elbow.normal(true)
               Thread.start(delta[1],mr999.elbow){|i,j| sleep_to_stop(i,j) }
           else
               mr999.elbow.reverse(true)
               Thread.start(delta[1],mr999.elbow){|i,j| sleep_to_stop(i,j) }
           end

        t3=if(deg.abs<CONVERGE_DEG)
              mr999.wrist.stop(true)
              nil
          elsif(delta[1]<-CONVERGE_DEG)
              mr999.wrist.normal(true)
              Thread.start(deg,mr999.wrist){|i,j| sleep_to_stop(i,j) }
          else
              mr999.wrist.reverse(true)
              Thread.start(deg,mr999.wrist){|i,j| sleep_to_stop(i,j) }
          end

        #
        # wait for joint stop
        #
        t1.join if t1
        t2.join if t2
        #t3.join if t3
    else
        mr999.stop
    end

    sleep(0.33)
}
