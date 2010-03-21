#
#
#
require 'rim'

module Rim
    class Image
        def to_gray
            return self unless self.channels==3

            if(self.typecode==NArray::BYTE)
                #
                # assume as BGR
                #
                buf=self.to_f
                buf[0,false]=buf*NVector.to_na([0.11,0.59,0.3])
                buf.plane(0).to_byte
            else
                #
                # assume as HSL
                #
                self[2,false].mul!(255).to_byte
            end
        end

        
        def to_hls
            return self unless self.typecode==NArray::BYTE 

            case self.channels
            when 1
                hls=self.class.new(self.width,self.height,3).to_f
                hls[1,false]=self.to_f.div!(255.0)
                return hls
            when 3
                _bgr_to_hls
            else
                return self
            end
        end

        def to_bgr
            if(self.typecode==NArray::BYTE && self.channels==1)
                self.add_plane([self,self,self])
            elsif(self.typecode==NArray::SFLOAT && self.channels==3)
                _hls_to_bgr
            else
                self
            end
        end


        def binarize(th,lt=0,ge=255)
            buf=self[]
            buf[self.lt(th)]=lt
            buf[self.ge(th)]=ge
            buf
        end

        def shift!(xoff=0,yoff=0,fill=0.0)
            xoff=xoff.to_i
            yoff=yoff.to_i
            
            return self if(xoff==0&&yoff==0)
            
            src_x=true
            dst_x=true
            src_y=true
            dst_y=true
            pad_x=nil
            pad_y=nil
            
            if(xoff>0)
                src_x=0..(-1-xoff)
                dst_x=xoff..-1
                pad_x=0..xoff-1
            elsif(xoff<0)
                src_x=-xoff..-1
                dst_x=0..(-1+xoff)
                pad_x=xoff..-1
            end
            if(yoff>0)
                src_y=0..(-1-yoff)
                dst_y=yoff..-1
                pad_y=0..yoff-1
            elsif(yoff<0)
                src_y=-yoff..-1
                dst_y=0..(-1+yoff)
                pad_y=yoff..-1
            end
            
            self[true,dst_x,dst_y]=self[true,src_x,src_y]
            if(fill)
                self[true,pad_x,true]=fill if pad_x
                self[true,true,pad_y]=fill if pad_y
            end
        
            self
        end
        
        def erosion(itr=1,neighbor=8)
            mask=self[]
            itr.times{
                mask[true,false]=mask.ne(0)
                buf=mask[]
                mask.mul!(buf[].shift!(-1,0))
                mask.mul!(buf[].shift!(1,0))
                mask.mul!(buf[].shift!(0,-1))
                mask.mul!(buf[].shift!(0,1))
                if(neighbor==8)
                    mask.mul!(buf[].shift!(-1,-1))
                    mask.mul!(buf[].shift!(-1,1))
                    mask.mul!(buf[].shift!(1,-1))
                    mask.mul!(buf[].shift!(1,1))
                end

                mask.mul!(255)
            }
            mask
        end

        def dilation(itr=1,neighbor=8)
            mask=self[]
            itr.times{
                mask[true,false]=mask.ne(0)
                buf=mask[]
                mask.add!(buf[].shift!(-1,0))
                mask.add!(buf[].shift!(1,0))
                mask.add!(buf[].shift!(0,-1))
                mask.add!(buf[].shift!(0,1))
                if(neighbor==8)
                    mask.add!(buf[].shift!(-1,-1))
                    mask.add!(buf[].shift!(-1,1))
                    mask.add!(buf[].shift!(1,-1))
                    mask.add!(buf[].shift!(1,1))
                end
                
                mask[mask.gt(0)]=255
            }
            mask
        end

        def gausian3x3
            d=self[].to_i.mul!(4)
            s=self[].to_i.mul!(2)
            d.add!(s[].shift!(-1,0))
            d.add!(s[].shift!(1,0))
            d.add!(s[].shift!(0,-1))
            d.add!(s[].shift!(0,1))

            s=self[].to_i
            d.add!(s[].shift!(-1,-1))
            d.add!(s[].shift!(-1,1))
            d.add!(s[].shift!(1,-1))
            d.add!(s[].shift!(1,1))

            d.div!(16)
        end

        def laplacian3x3
            s=self[].to_i
            d=self[].to_i.mul!(8)

            d.sbt!(s[].shift!(-1,0))
            d.sbt!(s[].shift!(1,0))
            d.sbt!(s[].shift!(0,-1))
            d.sbt!(s[].shift!(0,1))

            d.sbt!(s[].shift!(-1,-1))
            d.sbt!(s[].shift!(-1,1))
            d.sbt!(s[].shift!(1,-1))
            d.sbt!(s[].shift!(1,1))

            d
        end

        def pyramid_down(interpolate=:GAUSSIAN_3x3)
            case interpolate
            when :NONE
                _pyramid_down_none
            when :AVERAGE_2x2
                _pyramid_down_average_2x2
            when :GAUSSIAN_3x3
                _pyramid_down_gaussian_3x3
            else
                raise
            end
        end
        
        def pyramid_up(interpolate=:GAUSSIAN_3x3)
            case interpolate
            when :NONE,:AVERAGE_2x2
                _pyramid_up_none
            when :GAUSSIAN_3x3
                _pyramid_up_gaussian_3x3
            else
                raise
            end
        end
        
        private

        def _bgr_to_hls
            bgr=self.to_f.div!(255.0)
            hls=bgr[].fill!(0.0)

            max_l=bgr.max(0)
            min_l=bgr.min(0)

            max_pls_min=max_l+min_l
            max_mns_min=max_l-min_l

            #
            # L
            #
            hls[1,false]=max_pls_min/2.0

            #
            # S
            #
            buf=hls[2,false]
            mask=hls[1,false].le(0.5)
            buf[mask]=(max_mns_min/max_pls_min)[mask]

            mask=hls[1,false].gt(0.5)
            buf[mask]=(max_mns_min/(2.0-max_pls_min))[mask]
            buf[max_mns_min.eq(0.0)]=0.0

            hls[2,false]=buf


            #
            # H
            #
            c=bgr[].mul!(-1.0)
            c[0,false]=c[0,false].add!(max_l).div!(max_mns_min) # Cb
            c[1,false]=c[1,false].add!(max_l).div!(max_mns_min) # Cg
            c[2,false]=c[2,false].add!(max_l).div!(max_mns_min) # Cr

            buf=hls[0,false]

            mask=bgr[0,false].eq(max_l)
            buf[mask]=(c[1,false]-c[2,false]+4.0)[mask]

            mask=bgr[1,false].eq(max_l)
            buf[mask]=(c[2,false]-c[0,false]+2.0)[mask]

            mask=bgr[2,false].eq(max_l)
            buf[mask]=(c[0,false]-c[1,false])[mask]

            buf.mul!(NMath::PI).div!(3.0)
            mask=hls[0,false].lt(0.0)
            buf[mask]=buf[mask].add!(NMath::PI).add!(NMath::PI)
            buf[max_mns_min.eq(0.0)]=0.0
            
            hls[0,false]=buf

            hls
        end

        def _hls_to_bgr
            bgr=self.class.new(self.width,self.height,3).to_f

            max_l=self[1,false]*(self[2,false]+1.0)
            buf=self[1,false]*(-self[2,false]+1.0)+self[2,false]
            mask=self[1,false].gt(0.5)
            max_l[mask]=buf[mask]

            min_l=self[1,false]*2.0-max_l
            
            deg60=NMath::PI/3.0
            deg120=NMath::PI/3.0*2.0
            deg180=NMath::PI
            deg240=NMath::PI/3.0*4.0
            deg360=NMath::PI*2
            max_mns_min=max_l-min_l
            s_zero=self[2,false].eq(0.0)

            #
            # B
            #
            h=self[0,false]-deg120
            mask=h.lt(0.0)
            h[mask]+=deg360

            buf=bgr[0,false]
            mask=h.lt(deg60)
            buf[mask]=(min_l+(max_mns_min)*h/deg60)[mask]
            mask=h.ge(deg60).and(h.lt(deg180))
            buf[mask]=max_l[mask]
            mask=h.ge(deg180).and(h.lt(deg240))
            buf[mask]=(min_l+(max_mns_min)*(-h+deg240)/deg60)[mask]
            mask=h.ge(deg240)
            buf[mask]=min_l[mask]
            buf[s_zero]=0.0

            bgr[0,false]=buf

            #
            # G
            #
            h=self[0,false]

            buf=bgr[1,false]
            mask=h.lt(deg60)
            buf[mask]=(min_l+(max_mns_min)*h/deg60)[mask]
            mask=h.ge(deg60).and(h.lt(deg180))
            buf[mask]=max_l[mask]
            mask=h.ge(deg180).and(h.lt(deg240))
            buf[mask]=(min_l+(max_mns_min)*(-h+deg240)/deg60)[mask]
            mask=h.ge(deg240)
            buf[mask]=min_l[mask]
            buf[s_zero]=0.0

            bgr[1,false]=buf

            #
            # R
            #
            h=self[0,false]+deg120
            mask=h.gt(deg360)
            h[mask]-=deg360

            buf=bgr[2,false]
            mask=h.lt(deg60)
            buf[mask]=(min_l+(max_mns_min)*h/deg60)[mask]
            mask=h.ge(deg60).and(h.lt(deg180))
            buf[mask]=max_l[mask]
            mask=h.ge(deg180).and(h.lt(deg240))
            buf[mask]=(min_l+(max_mns_min)*(-h+deg240)/deg60)[mask]
            mask=h.ge(deg240)
            buf[mask]=min_l[mask]
            buf[s_zero]=0.0

            bgr[2,false]=buf

            bgr.mul!(255.0).to_byte
        end


        def _pyramid_down_none
            w=self.width/2
            h=self.height/2
            
            xmask=NArray.int(w).indgen!*2
            ymask=NArray.int(h).indgen!*2
            
            self[true,xmask,ymask]
        end
        
        def _pyramid_down_average_2x2
            t=self.typecode
            w=self.width/2
            h=self.height/2
            c=self.channels
            
            d=self[true,0..(w*2-1),0..(h*2-1)].to_f
            d=d.reshape!(c,2,w,2,h).sum(1,3).div!(4).reshape(w,h,c)
            
            case t
            when NArray::BYTE
                d.round.to_byte
            when NArray::SINT,NArray::INT
                d.round.to_type(t)
            else
                d.to_type(t)
            end
        end
        
        def _pyramid_down_gaussian_3x3
            t=self.typecode
            w=self.width/2
            h=self.height/2
            
            xmask=NArray.int(w).indgen!*2+1
            ymask=NArray.int(h).indgen!*2+1
            
            denominator=self[].to_f.fill!(1.0)
            denominator[true,xmask,true]=denominator[true,xmask,true].div!(2)
            denominator[true,true,ymask]=denominator[true,true,ymask].div!(2)
            
            numerator=self[].to_f.mul!(denominator)
            numerator.add!(numerator[].shift!(-1,0)+
                               numerator[].shift!(1,0))
            numerator.add!(numerator[].shift!(0,-1)+
                               numerator[].shift!(0,1))
            
            denominator.add!(denominator[].shift!(-1,0)+
                         denominator[].shift!(1,0))
            denominator.add!(denominator[].shift!(0,-1)+
                                 denominator[].shift!(0,1))
            
            d=numerator.div!(denominator)[true,xmask.sbt!(1),ymask.sbt!(1)]
            
            case t
            when NArray::BYTE
                d.round.to_byte
            when NArray::SINT,NArray::INT
                d.round.to_type(t)
            else
                d.to_type(t)
            end
        end
        
        def _pyramid_up_none
            t=self.typecode
            w=self.width*2
            h=self.height*2
            c=self.channels
            
            d=self.class.new(w,h,c).to_type(t)
            d.reshape!(c,2,w,2,h)
            
            2.times{|x|
                2.times{|y|
                    d[true,x,true,y,false]=self
                }
            }
            
            d.reshape!(c,w,h)
        end
        
        def _pyramid_up_gaussian_3x3
            t=self.typecode
            w=self.width*2
            h=self.height*2
            c=self.channels
            
            denominator=self.class.new(w,h,c).to_f
            
            xmask=NArray.int(self.width).indgen!*2
            ymask=NArray.int(self.height).indgen!*2
            
            numerator=self.class.new(w,h,c).to_f
            numerator[true,xmask,ymask]=self[].to_f
            numerator.add!(numerator[].shift!(-1,0)+
                               numerator[].shift!(1,0))
            numerator.add!(numerator[].shift!(0,-1)+
                               numerator[].shift!(0,1))
            
            denominator[true,xmask,ymask]=1.0
            denominator.add!(denominator[].shift!(-1,0)+
                                 denominator[].shift!(1,0))
            denominator.add!(denominator[].shift!(0,-1)+
                                 denominator[].shift!(0,1))
            
            d=numerator.div!(denominator)
            case t
            when NArray::BYTE
                d.round.to_byte
            when NArray::SINT,NArray::INT
                d.round.to_type(t)
            else
                d.to_type(t)
            end
        end

    end
end

require 'rim/rim_image_process'
