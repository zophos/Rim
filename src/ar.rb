#!/usr/bin/ruby
#
#
#
require 'rim'
require 'rim/rim_demo'
require 'gsl'

#
# camera resolusion
#
WIDTH=640.0
HEIGHT=480.0
HALF_HEIGHT=HEIGHT/2.0
IMG_CENTER=NVector.to_na([(WIDTH-1.0)/2,(HEIGHT-1.0)/2])

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


class Marker
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
    
    #
    # 2D Affine Rotation Matrix
    #
    ROT90=NMatrix.to_na([[0.0,1.0],[-1.0,0.0]])
    ROT180=NMatrix.to_na([[-1.0,0.0],[0.0,-1.0]])
    ROT270=NMatrix.to_na([[0.0,-1.0],[1.0,0.0]])

    def initialize(id,vertex,courner,img,work)
        @id=id
        _get_perspective_matrix(vertex)
        _mapping(courner)
        _sampling(img,work)
        _count_sample
        @rot=0
    end
    attr_reader :id,:world,:camera,:cog,:is_xy,:is_xz,:rot

    def size
        if(@world.dim>1)
            @world.shape[1]
        else
            0
        end
    end
    def is_xy?
        @is_xy>@is_xz
    end
    def is_xz?
        !self.is_xy?
    end
    
    def probability
        (@is_xy-@is_xz).to_f.abs/(@is_xy+@is_xz).to_f
    end

    def remap(other)
        if(self.is_xy?)
            _remap_xy(other)
        else
            _remap_xz(other)
        end
        self
    end

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

    private
    def _get_perspective_matrix(vertex)
        @cog=NVector.to_na(vertex).mean(1)

        #
        # sort rectangle vertex order lt->rt->lb->rb
        #
        vertex.sort!{|a,b|
            if((a[1]-cog[1])*(b[1]-cog[1])<0)
                a[1]<=>b[1]
            else
                a[0]<=>b[0]
            end
        }

        #
        # left-top is origin
        #
        @origin=vertex[0]

        #
        # calc destination squire's edge length
        #
        len=NMath.sqrt([(vertex[1]-vertex[0])**2,
                        (vertex[2]-vertex[0])**2,
                        (vertex[3]-vertex[1])**2,
                        (vertex[3]-vertex[2])**2,
                       ].sort.first)
        
        @magnify=60.0/len
        
        #
        # generate distination coord vector
        #
        warp_coord=NVector.float(2,4)

        warp_coord[0,true]=vertex[0][0]
        warp_coord[1,true]=vertex[0][1]

        warp_coord[0,1]+=len
        warp_coord[1,2]+=len
        warp_coord[true,3]=warp_coord[true,3].add!(len)
        
        #
        # solv parspective transform matrix
        #
        src_mat=NMatrix.float(8,8)
        4.times{|i|
            j=i*2
            src_mat[true,j..j+1]=
                NMatrix.to_na([[vertex[i][0],vertex[i][1],1.0,
                                      0.0,0.0,0.0,
                                      -vertex[i][0]*warp_coord[0,i],
                                      -vertex[i][1]*warp_coord[0,i]],
                                  [0.0,0.0,0.0,
                                      vertex[i][0],vertex[i][1],1.0,
                                      -vertex[i][0]*warp_coord[1,i],
                                      -vertex[i][1]*warp_coord[1,i]]])
        }
        r_vec=warp_coord.flatten/src_mat

        @rot3x3=NMatrix.float(3,3)
        @rot3x3[0..2,0]=r_vec[0..2]
        @rot3x3[0..2,1]=r_vec[3..5]
        @rot3x3[0..1,2]=r_vec[6..7]
        @rot3x3[2,2]=1.0
    end

    def _mapping(camera)
        vec=NVector.float(3)
        world=camera.map{|v|
            vec[0..1]=v
            vec[2]=1.0
            
            vec=@rot3x3*vec
            vec.div!(vec[2])
            
            t=vec[0..1].sbt!(@origin).mul!(@magnify).sbt!(30.0)

            gp_i=t.round.div!(5).mul!(5)
            gp=gp_i.to_f
            if(((gp_i%10).eq(0).any?) ||
               (gp[0].abs>25.0) ||
               (gp[1].abs>25.0) ||
               ((gp-t)**2)>9)
                nil
            else
                NVector.to_na([gp[0],-gp[1],0.0])
            end
        }.each_with_index{|w,i|
            camera[i]=nil unless w
        }

        camera.compact!
        world.compact!

        raise 'Not enough marker points found.' if camera.size<3

        @residual=nil
        @camera=NVector.to_na(camera)
        @world=NVector.to_na(world)
    end

    def _sampling(img,work)
        img.cv_warp_perspective(@rot3x3,work)

        sampling_points=SAMPLING_POINTS[].add!(30.0).div!(@magnify)
        sampling_points.add!(@origin)

        @sample=NMatrix.byte(5,5)
        i=0
        5.times{|y|
            5.times{|x|
                pt=sampling_points[true,i].round
                rx=(pt[0]-1)..(pt[0]+1)
                ry=(pt[1]-1)..(pt[1]+1)
                begin
                    @sample[x,y]=1 if work[0,rx,ry].eq(0).sum>4
                rescue
                end
                i+=1
            }
        }
    end

    def _count_sample
        @is_xy=@sample[0,0]+@sample[4,0]+
            @sample[1,1]+@sample[3,1]+
            @sample[1,3]+@sample[3,3]+
            @sample[0,4]+@sample[4,4]

        @is_xz=@sample[1,0]+@sample[3,0]+
            @sample[0,1]+@sample[4,1]+
            @sample[0,3]+@sample[4,3]+
            @sample[1,4]+@sample[3,4]
    end

    def _remap_xy(other)
        n=@sample[2,0]+@sample[2,4]+2-(@sample[0,2]+@sample[4,2])

        if(n>1)
            if(self.cog[1]>other.cog[1])
                @world[0..1,true]=ROT180*@world[0..1,true]
                @rot=180
            end
        else
            if(self.cog[0]<other.cog[0])
                @world[0..1,true]=ROT90*@world[0..1,true]
                @rot=90
            else
                @world[0..1,true]=ROT270*@world[0..1,true]
                @rot=270
            end
        end
        @world[2,true]=XY_PALNE_Z
    end

    def _remap_xz(other)
        n=@sample[1,2]+@sample[3,2]+2-(@sample[2,1]+@sample[2,3])

        if(n>1)
            if(self.cog[1]<other.cog[1])
                @world[0..1,true]=ROT180*@world[0..1,true]
                @rot=180
            end
        else
            if(self.cog[0]>other.cog[0])
                @world[0..1,true]=ROT90*@world[0..1,true]
                @rot=90
            else
                @world[0..1,true]=ROT270*@world[0..1,true]
                @rot=270
            end
        end

        @world[2,true]=@world[1,true].mul!(-1.0)
        @world[1,true]=XZ_PLANE_Y
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

    def set(view_angle,focal_point,z_axis,y_axis,color)
        self[0]=1.0
        self[1]=view_angle
        self[2..4]=focal_point
        self[5..7]=z_axis
        self[8..10]=y_axis
        self[11..13]=color
    end

    def send(*msg)
        self.set(*msg) unless msg.empty?
        @pipe.write(self)
    end
end


trap(:SIGINT){
    Rim::Terminal.close_all
    Rim.close_camera
    $stderr.write("\n")
    exit
}

camera=nil

if(ARGV[0])
    camera=Rim.load(ARGV[0])
else
    camera=Rim.open_camera(1,WIDTH.to_i,HEIGHT.to_i,15.0)
end

term=Rim::Terminal.open(camera.width,
                        camera.height,
                        'RimAr',
                        './glview/Debug/glview.exe -n %n -w %w -h %h')
cmd=GlCmd.new('RimAr_cmd',1024)

#
# recyclable buffers
#
h=NMatrix.float(4,3)
t1=Rim::Image.float(camera.width,camera.height)
t2=t1[]
gray=Rim::Image.new(camera.width,camera.height,1)
bin=gray[]
warp=gray[]

planes=[]

#
# debug buffers
#
dbg=nil
dbg_b=nil
dbg_g=nil
dbg_r=nil

term_dbg=Rim::Terminal.open(camera.width,
                            camera.height,
                            'Debug')

if(defined?(term_dbg) && term_dbg)
    dbg=Rim::Image.new(camera.width,camera.height,3)
end

loop{
    $stderr.write("#{Time.now}: ")
    camera.capture unless ARGV[0]
    
    cmd.clear

    camera.cv_bgr2gray(gray)
    gray.cv_binarize_ohtsu(bin)
    
    if(defined?(term_dbg) && term_dbg)
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
        vertex=bin.find_squire_vertex(15.0,50.0,0.5,3.0)
        next unless vertex
        

        if(defined?(term_dbg) && term_dbg)
            dbg_b[label_mask]=0
            dbg_g[label_mask]=255
            dbg[0..0,false]=dbg_b
            dbg[1..1,false]=dbg_g
        end

        #
        # get characteristic courners point
        #
        courners=gray.cv_find_courner_harris(t1,t2,bin.fill_hole)
        next unless courners.size>=3
        
        begin
            marker=Marker.new(l.id,vertex,courners,bin,warp)

            if(marker.size>=3)
                planes.push(marker) 

                if(defined?(term_dbg) && term_dbg)
                    dbg_b[label_mask]=255
                    dbg[0..0,false]=dbg_b
                end
            end
        rescue
        end

        if(planes.size==2)
            if((planes[0].is_xy?)^(planes[1].is_xy?))
                break
            else
                planes.sort!{|a,b| a.probability<=>b.probability }
                marker=planes.shift

                if(defined?(term_dbg) && term_dbg)
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

    if(defined?(term_dbg) && term_dbg)
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
        camera.show(term)
        cmd.send
        next
    end

    $stderr.write('maker found')
    planes[0].remap(planes[1])
    planes[1].remap(planes[0])
    
    h_fit_error=0.0
    loop{
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
            if(defined?(term_dbg) && term_dbg)
                c=c.round
                rx=(c[0]-1)..(c[0]+1)
                ry=(c[1]-1)..(c[1]+1)
                dbg[true,rx,ry]=GRN_SQR_3x3
                dbg.show(term_dbg)
            end
            
            $stderr.write('.')
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
    if(defined?(term_dbg) && term_dbg)
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
        camera.show(term)
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
    # send perspective parameters to OpenGL
    #
    camera.show(term)
    cmd.send(view_angle,focal_point,opt_axis,y_axis,TEAPOT_COLOR)

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
