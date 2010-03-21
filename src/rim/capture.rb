#
#
#
require 'rim'
require 'ewclib'

module Rim
    def self.open_camera(id,width,height,fps=15.0)
        Image.new(width,
                  height,
                  3).extend(EwcLib).open_camera(id,
                                                width,
                                                height,
                                                fps)
    end
    def self.close_camera
        EwcLib.close
    end
end
