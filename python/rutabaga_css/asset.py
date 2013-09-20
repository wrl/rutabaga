# rutabaga: an OpenGL widget toolkit
# Copyright (c) 2013 William Light.
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

all = [
    "RutabagaAsset",
    "RutabagaExternalAsset",
    "RutabagaEmbeddedAsset",
    
    "RutabagaEmbeddedTextureAsset",
    "RutabagaEmbeddedFontAsset"]

class RutabagaAsset(object):
    pass

class RutabagaExternalAsset(object):
    def __init__(self, path):
        self.path = path

    def __repr__(self):
        return '<{0.__class__.__name__} for {1}>'\
                .format(self, self.path)

class RutabagaEmbeddedAsset(object):
    def __init__(self, path, variable_name, prop=None):
        self.path = path
        self.variable_name = variable_name
        self.header_path = None
        self.prop = prop

    def __repr__(self):
        if self.header_path:
            return '<{0.__class__.__name__} embedding {1} as {2} (in {3})>'\
                    .format(self, self.path, self.variable_name,
                            self.header_path)
        else:
            return '<{0.__class__.__name__} embedding {1} as {2} (no header)>'\
                    .format(self, self.path, self.variable_name)

class RutabagaEmbeddedTextureAsset(RutabagaEmbeddedAsset):
    pass

class RutabagaEmbeddedFontAsset(RutabagaEmbeddedAsset):
    pass
