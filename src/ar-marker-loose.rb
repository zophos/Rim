#
# ar-marker-loose.rb
#
class Marker
    class Edge
        def initialize(id,p1,p2)
            @id=id
            @p1=p1
            @p2=p2
            
            @vec=NVector.float(4)
            @vec[0..1]=@p1
            @vec[2..3]=@p2-@p1
            @vec.div!(NMath.sqrt(@vec**2))
            
        end
        attr_reader :id,:p1,:p2,:vec
        
        def distance(other)
            @vec*other.vec
        end
    end

    @@seq=0

    #
    # marker grid points
    #
    WORLD_POINTS=NVector.to_na([[-30.0,30.0,0.0],
                                [30.0,30.0,0.0],
                                [-30.0,-30.0,0.0],
                                [30.0,-30.0,0.0]])
    def self.remap(m1,m2)
        r1=m1.base_edge_candidates(m2)
        r2=m2.base_edge_candidates(m1)
        
        max_pair=nil
        max_dist=nil
        
        r1.each{|e|
            r2.each{|f|
                d=e.distance(f)
                
                if(!max_dist || max_dist<d)
                    max_dist=d
                    max_pair=[e,f]
                end
            }
        }

        m1.remap(max_pair[0])
        m2.remap(max_pair[1])
    end

    def initialize(id,vertex,gray,bin,fill,t1,t2,warp)
        @seq=@@seq
        @@seq+=1

        @id=id
        @rot=0
        @is_xy=nil
        @is_xz=nil

        _mapping(vertex)
    end
    attr_reader :id,:world,:camera,:cog,:is_xy,:is_xz,:rot

    def size
        if(@world.dim>1)
            @world.shape[1]
        else
            0
        end
    end

    def edge(n)
        case n
        when 0
            Edge.new(n,@camera[true,0],@camera[true,1])
        when 1
            Edge.new(n,@camera[true,1],@camera[true,3])
        when 2
            Edge.new(n,@camera[true,2],@camera[true,3])
        when 3
            Edge.new(n,@camera[true,0],@camera[true,2])
        else
            nil
        end
    end

    def base_edge_candidates(other)
        if(self.cog[1]<other.cog[1])
            @is_xy=1
            @is_xz=0

            if(self.cog[0]<other.cog[0])
                [self.edge(2),self.edge(1)]
            else
                [self.edge(2),self.edge(3)]
            end
        else
            @is_xy=0
            @is_xz=1

            if(self.cog[0]<other.cog[0])
                [self.edge(0),self.edge(1)]
            else
                [self.edge(0),self.edge(3)]
            end
        end
    end


    def is_xy?
        if(@is_xy&&@is_xz)
            @is_xy>@is_xz
        else
            @seq%2==0
        end
    end

    def is_xz?
        !self.is_xy?
    end

    def probability
        1.0
    end

    def remap(e)
        if(@is_xy>@is_xz)
            _remap_xy(e.id)
        else
            _remap_xz(e.id)
        end

        self
    end

    private
    def _mapping(vertex)
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

        @residual=nil
        @camera=NVector.to_na(vertex)
        @world=WORLD_POINTS[]
    end


    def _remap_xy(e)
        case e
        when 0
            @world[0..1,true]=ROT180*@world[0..1,true]
            @rot=180
        when 1
            @world[0..1,true]=ROT90*@world[0..1,true]
            @rot=90
        when 2
            @rot=0
        when 3
            @world[0..1,true]=ROT270*@world[0..1,true]
            @rot=270
        end

        @world[2,true]=XY_PALNE_Z
    end

    def _remap_xz(e)
        case e
        when 0
            @rot=0
        when 1
            @world[0..1,true]=ROT270*@world[0..1,true]
            @rot=270
        when 2
            @world[0..1,true]=ROT180*@world[0..1,true]
            @rot=180
        when 3
            @world[0..1,true]=ROT90*@world[0..1,true]
            @rot=90
        end

        @world[2,true]=@world[1,true].mul!(-1.0)
        @world[1,true]=XZ_PLANE_Y
    end
end
