# rim/basic.rb -- basic classes of Rim
#
#
#
require 'narray'
require 'rim/range'

NaN=0.0/0.0
Inf=1.0/0.0

module Rim
    class Point2D<NVector
        def self.new(x,y)
            if(x.kind_of?(Numeric) && y.kind_of?(Numeric))
                self.to_na([x,y])
            else
                o=self.object(2)
                o[0]=x
                o[1]=y
                o
            end
        end
        def x
            self[0]
        end
        def x=(v)
            self[0]=v
        end
        def y
            self[1]
        end
        def y=(v)
            self[1]=v
        end
    end
    #
    # end of Rim::Point2D
    #

    class Point3D<Point2D
        def self.new(x,y,z)
            if(x.kind_of?(Numeric) &&
               y.kind_of?(Numeric) &&
               z.kind_of?(Numeric))
                self.to_na([x,y,z])
            else
                o=self.object(3)
                o[0]=x
                o[1]=y
                o[2]=z
                o
            end
        end
        def z
            self[2]
        end
        def z=(v)
            self[2]=v
        end
    end
    #
    # end of Rim::Point3D
    #

    class Rectangle
        def initialize(*args)
            if(args.nil?)
                @lt=@rb=Point2D.new(0,0)
            else
                case args.size
                when 1
                    if(Point2D===args[0])
                        if((Range===args[0].x)&&
                               (Range===args[0].y))
                            @lt=Point2D.new(args[0].x.first,args[0].y.first)
                            @rb=Point2D.new(args[0].x.last,args[0].y.last)
                        else(Point2D===args[0])
                            @lt=@rb=args[0]
                        end
                    else
                        raise
                    end
                when 2
                    if((Point2D===args[0])&&(Point2D===args[1]))
                        @lt=args[0]
                        @rb=args[1]
                    elsif((Range===args[0])&&(Range===args[1]))
                        @lt=Point2D.new(args[0].first,args[1].first)
                        @rb=Point2D.new(args[0].last,args[1].last)
                    else
                        raise
                    end
                when 3
                    if(Point2D===args[0])
                        @lt=args[0]
                        @rb=Point2D.new(@lt.x+args[1].to_i,
                                      @lt.y+args[2].to_i)
                    else
                        raise
                    end
                else
                    raise
                end
            end
        end
        attr_accessor :lt,:rb

        def range
            Point2D.new(@lt.x..@rb.x,@lt.y..@rb.y)
        end

        def width
            @rb.x-@lt.x+1
        end
        def height
            @rb.y-@lt.y+1
        end

        def to_a
            [@lt.x..@rb.x,@lt.y..@rb.y]
        end

        def center
            (@lt.to_f+@rb.to_f)/2.0
        end
    end
    #
    # end of Rim::Rectangle
    #

    class Image<NVector
        #
        # initialize method is always overwritten by NVector#initialize
        #
        def self.new(width,height,channel=1)
            self.byte(channel,width,height)
        end

        def channels
            self.shape[0]
        end
        def width
            self.shape[1]
        end
        def height
            self.shape[2]
        end
        
        def to_i
            self.to_type(NArray::LINT)
        end
        
        def to_byte
            buf=self[]
            buf[buf>255]=255
            buf[buf<0]=0
            buf.to_type(NArray::BYTE)
        end
        
        def plane(n)
            self.slice(n,false)
        end

        def add_plane(other)
            other=[other] unless other.kind_of?(Array)
            
            c=self.channels
            other.each{|o|
                c+=o.channels
            }

            buf=self.class.new(self.width,
                               self.height,
                               c).to_type(self.typecode)
            buf[0...self.channels,false]=self

            cs=ce=self.channels
            other.each{|o|
                raise ArgementError unless buf.width==o.width &&
                buf.height==o.height

                ce+=o.channels
                buf[cs...ce,false]=o
                cs+=o.channels
            }
            buf
        end
    end
    #
    # end of Rim::Image
    #
end
