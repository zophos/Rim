##########################################################################
# 
# shmemutex.rb -- shmemutex.dll wrapper class
#
#
require 'dl/import'

class ShMemutex
    module Api
        extend DL::Importable
    
        dlload('shmemutex.dll')
        
        extern(<<_EOS_
void *shmemutex_open(char *,long,int)
_EOS_
               )

        extern(<<_EOS_
void shmemutex_close(void *)
_EOS_
               )

        extern(<<_EOS_
char *shmemutex_name(void *)
_EOS_
               )

        extern(<<_EOS_
char *shmemutex_map_name(void *)
_EOS_
               )
        extern(<<_EOS_
char *shmemutex_mtx_name(void *)
_EOS_
               )

        extern(<<_EOS_
long shmemutex_map_size(void *)
_EOS_
               )

        extern(<<_EOS_
long shmemutex_lock(void *,long)
_EOS_
               )

        extern(<<_EOS_
long shmemutex_unlock(void *)
_EOS_
               )


        extern(<<_EOS_
long shmemutex_write(void *,void *,long,long)
_EOS_
               )

        extern(<<_EOS_
long shmemutex_read(void *,void *,long,long)
_EOS_
               )
    end
    #
    # end of Api
    #

    def initialize(share_name,maxsize,as_server=false)
        @handle=Api.shmemutex_open(share_name,maxsize,(as_server ? 1 : 0))
        @buf=DL.malloc(maxsize)
        ObjectSpace.define_finalizer(self,ShMemutex.finalize(@handle))
    end

    def close
        ObjectSpace.undefine_finalizer(self)
        Api.shmemutex_close(@handle) if @handle
        @handle=nil
        @buf=nil
    end

    def shared_name
        Api.shmemutex_name(@handle)
    end

    def map_name
        Api.shmemutex_map_name(@handle)
    end

    def mutex_name
        Api.shmemutex_mtx_name(@handle)
    end

    def mapped_size
        Api.shmemutex_map_size(@handle)
    end


    def lock(timeout=-1)
        Api.shmemutex_lock(@handle,timeout)
    end

    def unlock
        Api.shmemutex_unlock(@handle)
    end


    def write(data,timeout=0)
        data=data.to_s
        sz=[@buf.size,data.size].sort[0]
        @buf[0,sz]=data[0..sz]
        Api.shmemutex_write(@handle,@buf,sz,timeout)
        @buf.to_s(sz)
    end

    def read(timeout=-1)
        DL::PtrData.new(Api.shmemutex_read(@handle,@buf,@buf.size,timeout))
        @buf.to_s(@buf.size)
    end

    def ShMemutex.finalize(handle)
        Proc.new{
            Api.shmemutex_close(handle) if handle
        }
    end
end
