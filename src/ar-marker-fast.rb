#
# ar-marker-fast.rb
#
class Marker
    #
    # known world coord
    #
    WORLD_POINTS=NVector.to_na([[-30.0,30.0,0.0],
                                [30.0,30.0,0.0],
                                [-30.0,-30.0,0.0],
                                [30.0,-30.0,0.0]])
    def self.remap(m1,m2)
        m1.remap(m2)
        m2.remap(m1)
    end

    def initialize(id,vertex,gray,bin,fill,t1,t2,warp)
        @id=id
        @rot=0
        @is_xy=nil
        @is_xz=nil

        _get_perspective_matrix(vertex)
        _mapping(vertex)
        _sampling(bin,warp)
        _count_sample
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
        if(@is_xy+@is_xz==0)
            -1.0
        else
            (@is_xy-@is_xz).to_f.abs/(@is_xy+@is_xz).to_f
        end
    end

    def remap(o)
        if(self.is_xy?)
            _remap_xy(o)
        else
            _remap_xz(o)
        end

        self
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

    def _mapping(vertex)
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

        @residual=nil
        @camera=NVector.to_na(vertex)
        @world=WORLD_POINTS[]
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
