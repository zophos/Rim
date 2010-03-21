#!/usr/bin/ruby
#
#
#
require 'rim'
require 'rim/rim_demo'
require 'gsl'

WIDTH=640
HEIGHT=480

PAT_AREA_WORLD_VEC=NVector.to_na([[-30.0,30.0],
                         [30.0,30.0],
                         [-30.0,-30.0],
                         [30.0,-30.0]].flatten)
PAT_AREA_WORLD_MAT=PAT_AREA_WORLD_VEC.reshape(2,4)


XY_PALNE_Z=35.0
XZ_PLANE_Y=-35.0

SAMPLING_POINTS=NVector.to_na([[0.0,10.0,1.0],
                                  [10.0,10.0,1.0],
                                  [10.0,0.0,1.0],
                                  [10.0,-10.0,1.0],
                                  [0.0,-10.0,1.0],
                                  [-10.0,-10.0,1.0],
                                  [-10.0,0.0,1.0],
                                  [-10.0,10.0,1.0]])

ROT90=NMatrix.to_na([[0.0,1.0],[-1.0,0.0]])
ROT180=NMatrix.to_na([[-1.0,0.0],[0.0,-1.0]])
ROT270=NMatrix.to_na([[0.0,-1.0],[1.0,0.0]])

C_TOP=NVector.to_na([319.5,0.0])
C_BOTTOM=NVector.to_na([319.5,479.0])

RED_SQR_3x3=NVector.to_na([[[0,0,255],[0,0,255],[0,0,255]],
                           [[0,0,255],[0,0,255],[0,0,255]],
                           [[0,0,255],[0,0,255],[0,0,255]]])


class CoordMap
    def initialize(coord,rot3x3,cog,img)
        _mapping(coord,rot3x3,cog)
        _sampling(img,rot3x3,cog)
        _count_xy
    end
    attr_reader :world,:camera,:cog

    def coord_size
        @world.shape[1]
    end
    def is_xy?
        @is_xy>2
    end
    def is_xz?
        @is_xy<2
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

    private
    def _mapping(camera,rot3x3,cog)
        @cog=cog[]

        vec=NVector.float(3)
        world=camera.map{|v|
            vec[0..1]=v[].sbt!(@cog)
            vec[2]=1.0
            
            vec=rot3x3*vec
            vec.div!(vec[2])
            
            t=vec[0..1]
            gp_i=t.round.div!(5).mul!(5)
            gp=gp_i.to_f
            if(((gp_i%10).eq(0).any?) ||
               (gp[0].abs>25.0) ||
               (gp[1].abs>25.0) ||
               ((gp-t)**2)>9)
                nil
            else
                NVector.to_na([gp[0],gp[1],0.0])
            end
        }.each_with_index{|w,i|
            camera[i]=nil unless w
        }

        @camera=NVector.to_na(camera.compact)
        @world=NVector.to_na(world.compact)
    end

    def _sampling(img,rot3x3,cog)
        @sample=[]
        (0..7).each{|i|
            vertex=[]
            c=SAMPLING_POINTS[0..1,i]
            [-5,5].each{|oy|
                [-5,5].each{|ox|
                    x=c[0]+ox
                    y=c[1]+oy
                    mask=@world[0,true].eq(x).and(@world[1,true].eq(y))
                    pt=@camera[true,mask.where]
                    if(pt.size>0)
                        vertex.push(pt[true,0])
                    end
                }
            }
            if(vertex.size>1)
                pt=NArray.to_na(vertex).mean(1).round
                @sample.push(img[0,pt[0],pt[1]])
            else
                @sample.push(0)
            end
        }
    end

    def _count_xy
        t=@world.abs.eq(NVector.to_na([25.0,25.0,0.0]))
        @is_xy=(t[0,true]&t[1,true]).where.total
    end

    def _remap_xy(other)
        t=@world.abs.eq(NVector.to_na([5.0,25.0,0.0]))
        n=t[0,true]&t[1,true].where.total

        if(n>0)
            if(other.cog[1]<self.cog[1])
                @world[0..1,true]=ROT180*@world[0..1,true]
            end
        else
            if(other.cog[0]<self.cog[0])
                @world[0..1,true]=ROT90*@world[0..1,true]
            else
                @world[0..1,true]=ROT270*@world[0..1,true]
            end
        end
        @world[2,true]=XY_PALNE_Z
    end

    def _remap_xz(other)
        n=(@sample[0]+@sample[4]+255-@sample[2]+255-@sample[6])/255
        if(n>1)
            if(other.cog[1]>self.cog[1])
                @world[0..1,true]=ROT180*@world[0..1,true]
            end
        else
            if(other.cog[0]>self.cog[0])
                @world[0..1,true]=ROT90*@world[0..1,true]
            else
                @world[0..1,true]=ROT270*@world[0..1,true]
            end
        end

        @world[2,true]=@world[1,true]
        @world[1,true]=XZ_PLANE_Y
    end
end


camera=Rim.open_camera(1,WIDTH,HEIGHT,15.0)
#camera=Rim.load('t.bmp')
term=Rim::Terminal.open(camera.width,
                        camera.height,
                        'RimAr',
                        '/cygdrive/c/Users/zophos/Documents/work/rim/glview/Debug/glview.exe -n %n -w %w -h %h')
cmd=ShMemutex.new('RimAr_cmd',1024)

src_mat=NMatrix.float(8,8)
rot3x3=NMatrix.float(3,3)
rot3x3[2,2]=1.0

t1=Rim::Image.float(WIDTH,HEIGHT,1)
t2=t1[]

message=NArray.sfloat(8)
bin=camera.to_gray

term_dbg=Rim::Terminal.open(camera.width,
                        camera.height,
                        'Debug')
loop{
    camera.capture

    message.fill!(0.0)
    gray=camera.to_gray
    bin=gray.cv_binarize_ohtsu(bin)
    #bin=gray.cv_binarize_adaptive

    dbg=bin.add_plane([bin,bin])
    dbg[1..2,false]=0
    dbg_g=dbg[1..1,false]

    marker=[]
    
    label=bin.labeling(512)
    label.labelinfo.each{|l|
        #
        # check the area is marker area.
        # When that is marker area, get each vertex coordinates.
        #
        bin.fill!(0)[label.eq(l.id)]=255
        pt=bin.find_squire_vertex
        next unless pt
        
        dbg_g[label.eq(l.id)]=255
        dbg[1..1,false]=dbg_g
 
        #
        # calc parspective transform matrix
        #
        pt=NVector.to_na(pt).to_f
        cog=pt.mean(1)
        pt=pt.sbt!(cog).to_a.sort{|a,b|
            if(a[1]*b[1]<0)
                a[1]<=>b[1]
            else
                a[0]<=>b[0]
            end
        }
        pt=NVector.to_na(pt)

        4.times{|i|
            j=i*2
            src_mat[true,j..j+1]=
            NMatrix.to_na([[pt[0,i],pt[1,i],1.0,
                            0.0,0.0,0.0,
                            -pt[0,i]*PAT_AREA_WORLD_MAT[0,i],
                            -pt[1,i]*PAT_AREA_WORLD_MAT[0,i]],
                           [0.0,0.0,0.0,
                            pt[0,i],pt[1,i],1.0,
                            -pt[0,i]*PAT_AREA_WORLD_MAT[1,i],
                            -pt[1,i]*PAT_AREA_WORLD_MAT[1,i]]])
        }
        r_vec=PAT_AREA_WORLD_VEC/src_mat
        rot3x3[0..2,0]=r_vec[0..2]
        rot3x3[0..2,1]=r_vec[3..5]
        rot3x3[0..1,2]=r_vec[6..7]
        
        begin
            coordmap=
                CoordMap.new(gray.cv_find_courner_harris(t1,
                                                         t2,
                                                         bin.fill_hole),
                             rot3x3,
                             cog,
                             bin)
            marker.push(coordmap) if coordmap.coord_size>7

            if(coordmap.coord_size>0)
                coordmap.camera.shape[1].times{|i|
                    v=coordmap.camera[true,i][].round
                    dbg[true,(v[0]-1)..(v[0]+1),(v[1]-1)..(v[1]+1)]=RED_SQR_3x3
                }
            end
        rescue
            $stderr.write($!.to_s)
        end
    }

    dbg.show(term_dbg)

    if((marker.size!=2)||
       !((marker[0].is_xy?)^(marker[1].is_xy?)))
        
        camera.show(term)
        cmd.write(message)

       next
    end

    $stderr.write("maker found... ")
    marker[0].remap(marker[1])
    marker[1].remap(marker[0])
    
    #
    # prepare linear fit for H matrix 
    #
    n=(marker[0].coord_size+marker[1].coord_size)*2

    y=NVector.float(n)
    x=NMatrix.float(11,n)

    lim=marker[0].coord_size*2
    y[0...lim]=marker[0].dump_y
    x[true,0...lim]=marker[0].dump_x

    y[lim..-1]=marker[1].dump_y
    x[true,lim..-1]=marker[1].dump_x

    #
    # fit using GSL
    #
    (h11,cov,chisq,status)=GSL::MultiFit.linear(x.to_gm_view,y.to_gv_view)
    h11=h11.to_nv

    h=NMatrix.float(4,3)
    h[true,0]=h11[0..3]
    h[true,1]=h11[4..7]
    h[0..2,2]=h11[8..10]
    h[3,2]=1.0


    #
    # solve focul point
    #
    v=NVector.float(3)
    v[0]=-h[3,0]
    v[1]=-h[3,1]
    v[2]=-h[3,2]

    focal_point=v/h[0..2,true]

    #
    # optical axis vector
    #
    opt_axis=h[0..2,2]
    opt_axis.div!(NMath::sqrt((opt_axis**2).sum)).reshape!(3)
    
    #
    # Fixation point
    #
    fixation=
        NVector.to_na([focal_point[0]-focal_point[2]*opt_axis[0]/opt_axis[2],
                          focal_point[1]-focal_point[2]*opt_axis[1]/opt_axis[2],
                       0.0])


    #
    # viewing angle
    #
    ctop2x1=
        C_TOP/NMatrix.to_na([[h[0,0]-h[0,2]*C_TOP[0],
                              h[1,0]-h[1,2]*C_TOP[0]],
                             [h[0,1]-h[0,2]*C_TOP[1],
                              h[1,1]-h[1,2]*C_TOP[1]]])
    cbottom2x1=
        C_BOTTOM/NMatrix.to_na([[h[0,0]-h[0,2]*C_BOTTOM[0],
                                 h[1,0]-h[1,2]*C_BOTTOM[0]],
                                [h[0,1]-h[0,2]*C_BOTTOM[1],
                                 h[1,1]-h[1,2]*C_BOTTOM[1]]])
    ctop=NVector.to_na([ctop2x1[0],ctop2x1[1],0.0])
    cbottom=NVector.to_na([cbottom2x1[0],cbottom2x1[1],0.0])

    ctop.sbt!(focal_point)
    cbottom.sbt!(focal_point)

    ctop.div!(NMath.sqrt(ctop**2))
    cbottom.div!(NMath.sqrt(cbottom**2))
    
    view_angle=ctop*cbottom

    message[0]=1.0
    message[1]=Math.acos(view_angle)*180/Math::PI
    message[2..4]=focal_point
    message[5..7]=fixation


    camera.show(term)
    cmd.write(message)
    $stderr.write("solve H matrix: #{Time.now}\n")
}
