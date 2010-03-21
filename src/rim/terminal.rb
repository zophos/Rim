#
#
#
require 'rim'
require 'shmemutex'

module Rim
    class Terminal
        class Manager
            class RetryError < StandardError;end

            require 'singleton'
            include Singleton

            def initialize
                @dic={}
            end

            def add(term)
                @dic[term.__id__]=[term.pid,term.pipe,term] if term.pid
            end
            def delete(id)
                (pid,pipe,obj)=@dic.delete(id)
                obj=nil
                
                if(pid)
                    begin
                        Process.kill(:SIGTERM,pid)
                        Process.waitpid(pid)
                    rescue
                    end
                end
                pipe.close if pipe
                
                pid
            end

            def active_process
                @dic.keys.map{|id|
                    begin
                        obj=ObjectSpace._id2ref(id)
                        if(Process.waitpid(obj.pid,
                                           Process::WNOHANG||
                                               Process::WUNTRACED))
                            raise RangeError
                        end
                        obj
                    rescue RangeError
                        self.delete(id)
                        nil
                    end
                }.compact
            end

            def search_process_by_name(n)
                self.active_process.select{|o| 
                    o.share_name==n
                }[0]
            end

            def delete_by_name(n)
                obj=self.search_process_by_name(n)
                
                obj.close if obj
            end

            def search_process_by_size(width,height)
                self.active_process.select{|o| 
                    o.width==width && o.height==height
                }[0]
            end

            def delete_all
                @dic.keys.each{|id|
                    begin
                        ObjectSpace._id2ref(id).close
                    rescue RangeError,StandardError
                    end
                }
            end

            def generate_unique_name
                begin
                    n=sprintf('RimTerminal.%04x%08x%08x%04x',
                              Process.pid.to_i,
                              Thread.current.__id__,
                              Time.now.to_i,
                              rand(0xffff))
                    raise RetryError if self.search_process_by_name(n)
                rescue RetryError
                    sleep(0.01)
                    retry
                end
                n
            end
        end
        #
        # end of Rim::Terminal::Manager
        #

        @@default_viewer=
            '/cygdrive/c/Users/zophos/Documents/work/rim/rimshot/Debug/rimshot.exe -n %n -w %w -h %h'

        def self.default_viewer
            @@default_viewer
        end

        def self.default_viewer=(x)
            @@default_viewer=x.to_s
        end

        def self.manager
            Manager.instance
        end

        def self.open(width,height,share_name=nil,viewer=nil)
            self.new.open(width,height,share_name,viewer)
        end

        def self.close_by_name(name)
            Manager.instance.delete_by_name(name)
        end

        def self.close_all
            Manager.instance.delete_all
        end


        #
        # for finalizer
        #
        def self.finalize
            proc{|id|
                Manager.instance.delete(id)
            }
        end

        def initialize(viewer=nil)
            @viewer=viewer.to_s
            @pipe=nil
            @pid=nil

            @share_name=nil
            @width=0
            @height=0

            @manager=Terminal.manager
        end
        attr_reader :pid,:pipe,:share_name,:width,:height

        def viewer
            @viewer||@@default_viewer
        end
        def viewer=(x)
            if(x)
                @viewer=x.to_s
            else
                @viewer=nil
            end
        end

        def open(width,height,share_name=nil,viewer=nil)
            self.viewer=viewer

            share_name||=Manager.instance.generate_unique_name
            width=width.to_i
            height=height.to_i

            _start_process(share_name,width,height)

            @share_name=share_name
            @width=width
            @height=height

            Manager.instance.add(self)
            ObjectSpace.define_finalizer(self,Terminal.finalize)

            self
        end

        def close
            ObjectSpace.undefine_finalizer(self)

            Manager.instance.delete(self.__id__)
            @pipe=nil
            @pid=nil

            nil
        end

        def show(obj)
            raise 'No terminal opened.' unless (@pid && @pipe)

            #
            # make sure channels==3
            #
            obj=obj[0].add_plane(obj[1..-1]) if obj.kind_of?(Array)
            case obj.channels
            when 1
                obj=obj.add_plane([obj,obj])
            when 2
                obj=obj.add_plane(obj.plane(0).fill!(0))
            else
                obj=obj[0..2,false] if obj.channels>3
            end

            #
            # make sure typecode==BYTE
            #
            obj=obj.to_byte unless obj.typecode==NArray::BYTE
            
            #
            # adjust image size
            #
            unless(obj.width==@width && obj.height==@height)
                bw=true
                ow=(0...@width)
                if(obj.width<@width)
                    bw=(0...obj.width)
                    ow=true
                end
                bh=true
                oh=(0...@height)
                if(obj.height<@height)
                    bh=(0...obj.height)
                    oh=true
                end
                
                buf=Image.new(@width,@height,3)
                buf[true,bw,bh]=obj[true,ow,oh]

                obj=buf
            end

            #
            # check viewer is aliving
            #
            if(Process.waitpid(@pid,
                               Process::WNOHANG||Process::WUNTRACED))
                #
                # restart if died
                #
                @pipe.close if @pipe
                _start_process(@share_name,@width,@height)
                
                Manager.instance.add(self)
            end

            @pipe.write(obj)
        end

        private
        def _start_process(share_name,width,height)
            v=self.viewer.gsub(/%(.)/){|m|
                case $1
                when 'n'
                    share_name
                when 'w'
                    width.to_s
                when 'h'
                    height.to_s
                else
                    m
                end
            }
            
            begin
                @pid=Process.fork{ exec(v) }
            rescue
                @pid=nil
                if(@pipe)
                    @pipe.close
                    @pipe=nil
                end
                raise
            end
            @pipe=ShMemutex.new(share_name,width*height*3)
        end
    end
    #
    # end of Rim::Terminal
    #

    class Image<NVector
        def show(term=nil)
            if(term && !term.kind_of?(Terminal))
                term=Terminal.manager.search_process_by_name(term.to_s)||
                    Terminal.open(self.width,self.height,term.to_s)
            end

            term||=Terminal.manager.search_process_by_size(self.width,
                                                           self.height)
            term||=Terminal.open(self.width,self.height)

            term.show(self)
            term
        end

        def show_in_new_terminal
            self.show(Terminal.open(self.width,self.height))
        end
    end
end
