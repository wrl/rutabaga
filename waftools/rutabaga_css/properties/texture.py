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

from rutabaga_css.parser import ParseError
from rutabaga_css.asset import *
from rutabaga_css.prop import RutabagaStyleProperty

all = [
    "RutabagaTexture",
    "RutabagaExternalTexture",
    "RutabagaEmbeddedTexture",
    
    "RutabagaTextureProperty",
    "RutabagaBorderTextureProperty"]

class RutabagaTexture(object):
    def __init__(self, stylesheet, path):
        self.path = path
        self.stylesheet = stylesheet

class RutabagaExternalTexture(RutabagaTexture):
    def __init__(self, stylesheet, path):
        super(RutabagaExternalTexture, self).__init__(stylesheet, path)

        self.asset = RutabagaExternalAsset(path)
        self.stylesheet.external_assets.append(self.asset)

    c_repr_tpl = """\
\t\t\t\t\t.type = RTB_STYLE_PROP_TEXTURE,
\t\t\t\t\t.texture = {{
\t\t\t\t\t\t.location = RTB_ASSET_EXTERNAL,
\t\t\t\t\t\t.compression = RTB_ASSET_UNCOMPRESSED,
\t\t\t\t\t\t.external.path = "{0}",
{extra}}}"""

    def c_repr(self, extra=''):
        return self.c_repr_tpl.format(self.asset.path, extra=extra)

class RutabagaEmbeddedTexture(RutabagaTexture):
    def __init__(self, stylesheet, path):
        super(RutabagaEmbeddedTexture, self).__init__(stylesheet, path)

        self.width  = 0
        self.height = 0

        self.texture_var = sanitize_c_variable(path).upper()
        self.stylesheet.embedded_assets.append(
            RutabagaEmbeddedTextureAsset(path, self.texture_var, self))

    c_repr_tpl = """\
\t\t\t\t\t.type = RTB_STYLE_PROP_TEXTURE,
\t\t\t\t\t.texture = {{
\t\t\t\t\t\t.location = RTB_ASSET_EMBEDDED,
\t\t\t\t\t\t.compression = RTB_ASSET_UNCOMPRESSED,
\t\t\t\t\t\t.embedded.base = {var},
\t\t\t\t\t\t.embedded.size = sizeof({var}),
\t\t\t\t\t\t.w = {width},
\t\t\t\t\t\t.h = {height},
{extra}}}"""

    def c_repr(self, extra=''):
        return self.c_repr_tpl.format(
            var=self.texture_var,
            width=self.width,
            height=self.height,
            extra=extra)

c_repr_extra = """\
\t\t\t\t\t\t.flags  = {flags},
\t\t\t\t\t\t.border = {{
\t\t\t\t\t\t\t.top    = {borders[0]},
\t\t\t\t\t\t\t.right  = {borders[1]},
\t\t\t\t\t\t\t.bottom = {borders[2]},
\t\t\t\t\t\t\t.left   = {borders[3]}
\t\t\t\t\t\t}}"""

class RutabagaTextureProperty(RutabagaStyleProperty):
    default_method = RutabagaEmbeddedTexture
    method_map = {
        "embed": RutabagaEmbeddedTexture,
        "extern": RutabagaExternalTexture}

    def parse_extra_tokens(self, tokens):
        pass

    def add_flag(self, flag):
        if not self.flags:
            self.flags = flag
        else:
            self.flags += ' | ' + flags

    def __init__(self, stylesheet, name, tokens):
        self.path = None
        self.method = RutabagaTextureProperty.default_method
        self.borders = [0,0,0,0]
        self.flags   = None

        tok = tokens[0]

        if tok.type == "URI":
            self.path = tok.value

        elif tok.type == "FUNCTION":
            method_map = RutabagaTextureProperty.method_map
            self.path = tok.content[0].value

            try:
                self.method = method_map[tok.function_name]
            except KeyError:
                raise ParseError(tok,
                    "valid texture reference functions are {0}, and url()"\
                        .format(", ".join(
                            ["{0}()".format(f) for f in method_map])))

        self.texture = self.method(stylesheet, self.path)
        self.parse_extra_tokens(tokens[1:])

    def c_repr(self):
        extra = c_repr_extra.format(
                borders=self.borders,
                flags=self.flags or '0')
        return self.texture.c_repr(extra)

def chained_takewhile(iterable, *preds):
    ret = [[] for _ in range(len(preds))]

    iterable = iter(iterable)

    try:
        item = next(iterable)

        for (pred, ary) in zip(preds, ret):
            while True:
                if not pred(item):
                    break

                ary.append(item)
                item = next(iterable)
    except StopIteration:
        pass

    return ret

class RutabagaBorderTextureProperty(RutabagaTextureProperty):
    def parse_extra_tokens(self, tokens):
        borders, fill, repeat = chained_takewhile(tokens,
                lambda x: x.type in ['DIMENSION', 'S'],
                lambda x: x.type == 'IDENT' and x.value == 'fill',
                lambda x: x.type == 'IDENT' and x.value[:6] == 'repeat')

        slices = list(filter(lambda x: x.type == 'DIMENSION', borders))
        non_px = list(filter(lambda x: x.unit != 'px', slices))
        if non_px:
            raise ParseError(non_px[0],
                    'slices can only be specified in pixels, sorry')

        b = lambda *x: [slices[i].value for i in x]
        if len(slices) == 1:
            self.borders = b(0, 0, 0, 0)
        elif len(slices) == 2:
            self.borders = b(1, 0, 1, 0)
        elif len(slices) == 3:
            self.borders = b(0, 1, 2, 1)
        elif len(slices) == 4:
            self.borders = b(0, 1, 2, 3)

        if fill:
            self.add_flag('RTB_TEXTURE_FILL')
