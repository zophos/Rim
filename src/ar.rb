#!/usr/bin/ruby
#
#
#
require 'gsl'
require 'rim'
require 'rim/rim_demo'

class Marker

    #
    # 2D Affine Rotation Matrix
    #
    ROT90=NMatrix.to_na([[0.0,1.0],[-1.0,0.0]])
    ROT180=NMatrix.to_na([[-1.0,0.0],[0.0,-1.0]])
    ROT270=NMatrix.to_na([[0.0,-1.0],[1.0,0.0]])

    #
    # marker grid points
    #
    SAMPLING_POINTS=NVector.to_na([[-20.0,-20.0],
                                   [-10.0,-20.0],
                                   [0.0,-20.0],
                                   [10.0,-20.0],
                                   [20.0,-20.0],
                                   [-20.0,-10.0],
                                   [-10.0,-10.0],
                                   [0.0,-10.0],
                                   [10.0,-10.0],
                                   [20.0,-10.0],
                                   [-20.0,0.0],
                                   [-10.0,0.0],
                                   [0.0,0.0],
                                   [10.0,0.0],
                                   [20.0,0.0],
                                   [-20.0,10.0],
                                   [-10.0,10.0],
                                   [0.0,10.0],
                                   [10.0,10.0],
                                   [20.0,10.0],
                                   [-20.0,20.0],
                                   [-10.0,20.0],
                                   [0.0,20.0],
                                   [10.0,20.0],
                                   [20.0,20.0]])

    XY_PALNE_Z=-35.0
    XZ_PLANE_Y=-35.0


    def dump_x
        rows=@world.shape[1]
        buf=NMatrix.float(11,rows*2)
        rows.times{|r|
            j=r*2
            buf[true,j..j+1]=NMatrix.to_na([[@world[0,r],
                                             @world[1,r],
                                             @world[2,r],
                                             1.0,
                                             0.0,0.0,0.0,0.0,
                                             -@world[0,r]*@camera[0,r],
                                             -@world[1,r]*@camera[0,r],
                                             -@world[2,r]*@camera[0,r]],
                                            [0.0,0.0,0.0,0.0,
                                             @world[0,r],
                                             @world[1,r],
                                             @world[2,r],
                                             1.0,
                                             -@world[0,r]*@camera[1,r],
                                             -@world[1,r]*@camera[1,r],
                                             -@world[2,r]*@camera[1,r]]])
        }
        buf
    end

    def dump_y
        @camera.reshape(@camera.shape[1]*2)
    end

    def residual(h)
        world=NVector.float(4,@world.shape[1])
        world[0..2,true]=@world
        world[3,true]=1.0

        sol=h*world
        sol.div!(sol[2..2,true])
        @residual=sol[0..1,true].sbt!(@camera)
        @residual=@residual**2

        idx=@residual.sort_index
        @camera=@camera[true,idx]
        @world=@world[true,idx]

        @residual
    end

    def delete_max_residual_row
        raise 'Too few points remains' if self.size<4
        rej=[@camera[true,-1],@world[true,-1]]

        @camera=@camera[true,0..-2]
        @world=@world[true,0..-2]

        rej
    end
end

class GlCmd<NArray
    def self.finalize(shm)
        proc{|id|
            shm.close if shm
        }
    end

    def self.new(*shm_opts)
        shm=ShMemutex.new(*shm_opts)
        obj=self.sfloat(16)
        obj.instance_variable_set(:@pipe,shm)
        ObjectSpace.define_finalizer(obj,self.finalize(shm))

        obj
    end

    def initialize
        @pipe=nil
    end

    def clear
        self.fill!(0.0)
    end

    def set(view_angle,focal_point,z_axis,y_axis,color,water_width=0.0)
        self[0]=1.0
        self[1]=view_angle
        self[2..4]=focal_point
        self[5..7]=z_axis
        self[8..10]=y_axis
        self[11..13]=color
        self[14]=water_width
    end

    def send(*msg)
        self.set(*msg) unless msg.empty?
        @pipe.write(self)
    end
end


#
# camera resolusion
#
WIDTH=640.0
HEIGHT=480.0
HALF_HEIGHT=HEIGHT/2.0

#
# colors
#
TEAPOT_COLOR=NVector.to_na([1.0,1.0,1.0])


#
# debuging markers
#
RED_SQR_3x3=NVector.to_na([[[0,0,255],
                            [0,0,255],
                            [0,0,255]],
                           [[0,0,255],
                            [0,0,255],
                            [0,0,255]],
                           [[0,0,255],
                            [0,0,255],
                            [0,0,255]]])
GRN_SQR_3x3=NVector.to_na([[[0,255,0],
                            [0,255,0],
                            [0,255,0]],
                           [[0,255,0],
                            [0,255,0],
                            [0,255,0]],
                           [[0,255,0],
                            [0,255,0],
                            [0,255,0]]])
YLW_SQR_5x5=NVector.to_na([[[0,0,0],
                            [0,0,0],
                            [0,255,255],
                            [0,0,0],
                            [0,0,0]],
                           [[0,0,0],
                            [0,0,0],
                            [0,255,255],
                            [0,0,0],
                            [0,0,0]],
                           [[0,255,255],
                            [0,255,255],
                            [0,255,255],
                            [0,255,255],
                            [0,255,255]],
                           [[0,0,0],
                            [0,0,0],
                            [0,255,255],
                            [0,0,0],
                            [0,0,0]],
                           [[0,0,0],
                            [0,0,0],
                            [0,255,255],
                            [0,0,0],
                            [0,0,0]]])

COS45=Math.cos(Math::PI/4.0)
SQRT2=Math.sqrt(2.0)


DEFUALT_MEHOTD=:strict
DEFAULT_CAMERA=1


method=DEFUALT_MEHOTD
camera_id=DEFAULT_CAMERA
filename=nil
debug=nil
noterminal=nil
checkpoint=nil

while(o=ARGV.shift)
    case o
    when '-m'
        case ARGV.shift[0..0].to_s
        when 'l'
            method=:loose
        when 'f'
            method=:fast
        when 's'
            method=:strict
        else
            method=DEFUALT_MEHOTD
        end
    when '-d'
        debug=true
    when '-c'
        camera_id=ARGV.shift.to_i
        filename=nil
    when '-f'
        camera_id=nil
        filename=ARGV.shift
    when '--no-terminal'
        noterminal=true
    else
        # nop
    end
end


case(method)
when :loose
    require 'ar-marker-loose'
when :fast
    require 'ar-marker-fast'
else
    require 'ar-marker-strict'
end

trap(:SIGINT){
    Rim::Terminal.close_all
    Rim.close_camera
    $stderr.write("\n")
    exit
}

camera=nil
if(filename)
    camera=Rim.load(filename)
else
    camera=Rim.open_camera(camera_id,WIDTH.to_i,HEIGHT.to_i,5.0)
end


#
# terminals
#
term=nil
term_dbg=nil

unless(noterminal)
    term=Rim::Terminal.open(camera.width,
                            camera.height,
                            'RimAr',
                            './glview/Debug/glview.exe -n %n -w %w -h %h')
    term_dbg=Rim::Terminal.open(camera.width,
                                camera.height,
                                'Debug') if debug
end
cmd=GlCmd.new('RimAr_cmd',1024)


#
# recyclable buffers
#
planes=[]
h=NMatrix.float(4,3)
gray=Rim::Image.new(camera.width,camera.height,1)
bin=gray[]
t1=nil
t2=nil
fill=nil
warp=nil
if(method==:strict)
    t1=Rim::Image.float(camera.width,camera.height)
    t2=t1[]
    fill=gray[]
end
if(method!=:loose)
    warp=gray[]
end


#
# debug buffers
#
dbg=nil
dbg_b=nil
dbg_g=nil
dbg_r=nil

if(term_dbg)
    dbg=Rim::Image.new(camera.width,camera.height,3)
end


loop{
    #
    #
    $stderr.write("#{Time.now}: ")

    camera.capture if camera.respond_to?(:capture)
    cmd.clear

    camera.cv_bgr2gray(gray)
    gray.cv_binarize_ohtsu(bin)

    if(term_dbg)
        dbg[0..0,false]=bin
        dbg[1..2,false]=0
        dbg_b=dbg[0..0,false]
        dbg_g=dbg[1..1,false]
        dbg_r=dbg[2..2,false]
    end


    planes.clear
    label=bin.labeling(512)
    label.labelinfo.each{|l|
        label_mask=label.eq(l.id)

        #
        # check the area is marker area.
        # When that is marker area, get each vertex coordinates.
        #
        bin.fill!(0)[label_mask]=255
        vertex=bin.find_squire_vertex(15.0,55.0,0.4,5.0)
        next unless vertex

        if(term_dbg)
            dbg_b[label_mask]=0
            dbg_g[label_mask]=255
            dbg[0..0,false]=dbg_b
            dbg[1..1,false]=dbg_g
        end

        begin
            planes.push(Marker.new(l.id,vertex,gray,bin,fill,t1,t2,warp)) 

            if(term_dbg)
                dbg_b[label_mask]=255
                dbg[0..0,false]=dbg_b
            end
        rescue
            next
        end

        if(planes.size==2)
            if((planes[0].is_xy?)^(planes[1].is_xy?))
                break
            else
                planes.sort!{|a,b|
                    a.probability <=> b.probability
                }
                marker=planes.shift

                if(term_dbg)
                    label_mask=label.eq(marker.id)
                    dbg_b[label_mask]=0
                    dbg_g[label_mask]=0
                    dbg_r[label_mask]=255
                    dbg[0..0,false]=dbg_b
                    dbg[1..1,false]=dbg_g
                    dbg[2..2,false]=dbg_r
                end
            end
        end
    }

    if(term_dbg)
        planes.each{|pln|
            pln.size.times{|i|
                v=pln.camera[true,i].round
                rx=(v[0]-1)..(v[0]+1)
                ry=(v[1]-1)..(v[1]+1)
                dbg[true,rx,ry]=RED_SQR_3x3
            }
        }

        dbg.show(term_dbg)
    end


    err_msg=nil
    if(planes.size!=2)
        err_msg=''
    elsif(planes[0].size+planes[0].size<6)
        err_msg='Too few marker points.'
    elsif(!((planes[0].is_xy?)^(planes[1].is_xy?)))
        err_msg="Seems two planes are same.\n"+
            " [0] #{planes[0].is_xy}, #{planes[0].is_xz}\n"+
            " [1] #{planes[1].is_xy}, #{planes[1].is_xz}"
    end
    if(err_msg)
        $stderr.write(err_msg)
        $stderr.write("\n")
        camera.show(term) if term
        cmd.send
        next
    end

    Marker.remap(planes[0],planes[1])

    h_fit_error=0.0
    loop{
        $stderr.write('.')

        #
        # prepare linear fit for H matrix 
        #
        n=(planes[0].size+planes[1].size)*2
        
        x=NMatrix.float(11,n)
        y=NVector.float(n)
        
        lim=planes[0].size*2
        x[true,0...lim]=planes[0].dump_x
        y[0...lim]=planes[0].dump_y
        
        x[true,lim..-1]=planes[1].dump_x
        y[lim..-1]=planes[1].dump_y

        #
        # liner fit using GSL
        #
        (h11,cov,chisq,status)=GSL::MultiFit.linear(x.to_gm_view,y.to_gv_view)
        h11=h11.to_nv_ref
        
        h[true,0]=h11[0..3]
        h[true,1]=h11[4..7]
        h[0..2,2]=h11[8..10]
        h[3,2]=1.0
        
        residula=NArray.float(n/2)
        lim=planes[0].size
        residula[0...lim]=planes[0].residual(h)
        residula[lim..-1]=planes[1].residual(h)
        h_fit_error=residula.mean

        break if h_fit_error<1.0 || n<13

        i=(residula.sort_index[-1]<lim ? 0 : 1 )
        begin
            (c,)=planes[i].delete_max_residual_row
            if(term_dbg)
                c=c.round
                rx=(c[0]-1)..(c[0]+1)
                ry=(c[1]-1)..(c[1]+1)
                dbg[true,rx,ry]=GRN_SQR_3x3
                dbg.show(term_dbg)
            end
        rescue
            $stderr.write($!.to_s)
            break
        end
    }
    
    #
    # world origin point on camera coordinate system
    #
    # (x,y,1)^T=H.(0,0,0,1)^T
    #
    if(term_dbg)
        world_origin=h[3,0..1].round

        rx=(world_origin[0]-2)..(world_origin[0]+2)
        ry=(world_origin[1]-2)..(world_origin[1]+2)
        begin
            dbg[true,rx,ry]=dbg[true,rx,ry]|YLW_SQR_5x5
        rescue
            $stderr.write(' Odd H matrix! ')
        end
        dbg.show(term_dbg)
    end

    #
    # Focul Point
    #
    # H.(x,y,z,1)^T=O
    #
    v=NVector.float(3)
    v[0]=-h[3,0]
    v[1]=-h[3,1]
    v[2]=-h[3,2]
    
    begin
        focal_point=v/h[0..2,true]
    rescue
        $stderr.write($!.to_s)
        $stderr.write("\n")
        camera.show(term) if term
        cmd.send
        next
    end

    #
    # Optical Axis Vector and Z axis
    #
    # (H31,H32,H33)/||(H31,H32,H33)||
    #
    q=h[0..2,0..2]
    alpha=NMath::sqrt(NVector.to_na(q[0..2,2].to_a.flatten)**2)
    q.div!(alpha)
    opt_axis=NVector.to_na(q[0..2,2].to_a.flatten)
    opt_axis.mul!(-1.0) if opt_axis[2]>0.0
    
    z_axis=opt_axis[].mul!(-1.0)


    #
    # Focal Length (in pixel) and X axis
    #
    x_axis=NVector.to_na(q[0..2,0].to_a.flatten)
    x_axis.sbt!(opt_axis*(x_axis*opt_axis))
    focal_length=NMath.sqrt(x_axis**2)

    x_axis.div!(focal_length)

    #
    # View Angle
    #
    t=HALF_HEIGHT/focal_length
    view_angle=NMath.atan((t*2.0)/(1-t*t))*180.0/NMath::PI
    
    #
    # Y axis
    #
    # cross product of Z axis and X axis
    #
    y_axis=NVector.to_na([z_axis[1]*x_axis[2]-z_axis[2]*x_axis[1],
                             z_axis[2]*x_axis[0]-z_axis[0]*x_axis[2],
                             z_axis[0]*x_axis[1]-z_axis[1]*x_axis[0]])


    #
    # appendix :)
    #
    water_width=0.0
    water_width=1.0+4.0/(1.0-SQRT2)*(SQRT2*x_axis[1]+1) if x_axis[1]<=-COS45
    water_width=0.0 if water_width>5.0 

    #
    # send perspective parameters to OpenGL
    #
    camera.show(term) if term

    if(h_fit_error<10.0)
        cmd.send(view_angle,
                 focal_point,
                 opt_axis,
                 y_axis,
                 TEAPOT_COLOR,
                 water_width)
    else
        cmd.send
    end


    $stderr<< <<_EOS_
:
  Marker 0: #{planes[0].size} #{planes[0].is_xy} #{planes[0].is_xz} #{planes[0].rot}
  Marker 1: #{planes[1].size} #{planes[1].is_xy} #{planes[1].is_xz} #{planes[1].rot}

  H: #{h.to_a.map{|x| x.inspect}.join("\n     ")}
  Fitting Error:  #{h_fit_error}

  Forcal Point:   #{focal_point.to_a.inspect}
  X Axis:         #{x_axis.to_a.inspect}
  Y Axis:         #{y_axis.to_a.inspect}
  Z Axis:         #{z_axis.to_a.inspect}
  Viewing Angle:  #{view_angle}
  Mean Depth:     #{NMath.sqrt(focal_point**2)}

_EOS_

}
