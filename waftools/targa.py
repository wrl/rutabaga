# rutabaga: an OpenGL widget toolkit
# Copyright (c) 2013-2018 William Light.
# All rights reserved.
#
# This is free and unencumbered software released into the public domain.
#
# Anyone is free to copy, modify, publish, use, compile, sell, or
# distribute this software, either in source code form or as a compiled
# binary, for any purpose, commercial or non-commercial, and by any
# means.
#
# In jurisdictions that recognize copyright laws, the author or authors
# of this software dedicate any and all copyright interest in the
# software to the public domain. We make this dedication for the benefit
# of the public at large and to the detriment of our heirs and
# successors. We intend this dedication to be an overt act of
# relinquishment in perpetuity of all present and future rights to this
# software under copyright law.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
# OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
# For more information, please refer to <http://unlicense.org/>

from collections import OrderedDict
from struct import Struct

all = [
    "TargaLoadException",

    "TargaHeader",
    "TargaImage"]

tga_header = Struct(
    "<" # little endian

    "B" # id length
    "B" # cmap type
    "B" # img type

    "H" # cmap offset
    "H" # cmap length
    "B" # cmap bits per pixel

    "H" # origin X
    "H" # origin Y
    "H" # width
    "H" # height
    "B" # bits per pixel

    "B" # pad:2
        # origin:2
        # alpha_bits:4
    )

class TargaLoadException(Exception):
    pass

class TargaHeader(object):
    def __init__(self):
        from itertools import repeat

        self.id_length,           \
                self.cmap_type,   \
                self.img_type,    \
                self.cmap_offset, \
                self.cmap_len,    \
                self.cmap_bpp,    \
                self.origin_x,    \
                self.origin_y,    \
                self.width,       \
                self.height,      \
                self.bpp,         \
                self.origin,      \
                self.alpha_bits   \
            = repeat(0, 13)

    def from_struct_tuple(self, tup):
        self.id_length,           \
                self.cmap_type,   \
                self.img_type,    \
                self.cmap_offset, \
                self.cmap_len,    \
                self.cmap_bpp,    \
                self.origin_x,    \
                self.origin_y,    \
                self.width,       \
                self.height,      \
                self.bpp,         \
                extras            \
            = tup

        self.origin = extras & 0x30
        self.alpha_bits = extras & 0x0F

    def from_bytes(self, bstring):
        self.from_struct_tuple(tga_header.unpack(bstring))

class TargaImage(TargaHeader):
    # subclasses from TargaHeader instead of just having one as an instance
    # var so that queries from outside the class are nicer.
    # i.e.:
    #     img = TargaImage()
    #     img.width
    # vs
    #     img = TargaImage()
    #     img.headerwidth
    #
    # just a preference thing.

    def __init__(self):
        super(TargaImage, self).__init__()

        self.header = TargaHeader()
        self.data = b""

    def verify_simple_tga(self):
        if self.id_length != 0 or self.cmap_type != 0 or self.img_type != 2:
            raise TargaLoadException("unsupported TGA image type")

    def from_bytes(self, bstring):
        super(TargaImage, self).from_bytes(bstring[:tga_header.size])

        self.verify_simple_tga()
        self.data = bstring[tga_header.size:]
