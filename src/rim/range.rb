#
#
#
class Range
    alias _each each unless defined?(_each)
    def each(&block)
        if((self.first<=>self.last)==1 && self.first.respond_to?(:downto))
            self.first.downto(self.last){|x|
                yield(x)
            }
            self
        else
            _each(&block)
        end
    end

    def reverse
        if(self.exclude_end?)
            self.last...self.first
        else
            self.last..self.first
        end
    end

    def and(other)
        #[self.min,other.min].max..[self.max,other.max].min  # too slow

        s=[self.first,other.first].max
        e=[self.last,other.last].min
        if((e==self.last && self.exclude_end?)||
               (e==other.last && other.exclude_end?))
            s...e
        else
            s..e
        end
    end

    def or(other)
        #[self.min,other.min].min..[self.max,other.max].max

        s=[self.first,other.first].min
        e=[self.last,other.last].max
        if((e==self.last && self.exclude_end?)||
               (e==other.last && other.exclude_end?))
            s...e
        else
            s..e
        end
    end
end
