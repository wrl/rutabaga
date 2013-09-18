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

from collections import OrderedDict

from rutabaga_css.parser import ParseError

from rutabaga_css.properties.rgba import *
from rutabaga_css.properties.texture import *

all = ["RutabagaStyle"]

state_mapping = {
    "normal": "RTB_DRAW_NORMAL",
    "focus":  "RTB_DRAW_FOCUS",
    "hover":  "RTB_DRAW_HOVER",
    "active": "RTB_DRAW_ACTIVE"
}

available_states = {
    "normal": "RTB_STYLE_NORMAL",
    "focus":  "RTB_STYLE_FOCUS",
    "hover":  "RTB_STYLE_HOVER",
    "active": "RTB_STYLE_ACTIVE"
}

prop_mapping = {
    "color": RutabagaRGBAProperty,
    "background-color": RutabagaRGBAProperty,
    "background-image": RutabagaTextureProperty
}

class RutabagaStyle(object):
    def __init__(self, stylesheet, type, normal_props):
        self.stylesheet = stylesheet

        self.type = type
        self.states = {}

        self.add_state("normal", normal_props)

    def __repr__(self):
        return ('<{0.__class__.__name__} for {0.type}>'.format(self))

    def add_state(self, state, styles):
        self.states[state] = OrderedDict()

        if state not in state_mapping.keys():
            raise AttributeError('"{0}" is not a valid state'.format(state))

        for s in styles:
            try:
                self.states[state][s] = \
                    prop_mapping[s](self.stylesheet, s, styles[s])
            except KeyError:
                raise ParseError(styles[s][0],
                        'unknown property "{0}"'.format(s))

    c_state_repr = """\
\t\t\t[{state}] = (struct rtb_style_property_definition []) {{
{styles},

\t\t\t\t{{NULL}}
\t\t\t}}"""

    c_style_repr = """\
\t{{"{type}",
\t\t{styles},

\t\t.properties = {{
{states}
\t\t}}
\t}}"""

    c_prop_repr = """\
\t\t\t\t{{"{0}",
{1}}}"""

    def c_repr(self):
        states = [self.c_state_repr.format(
            state=state_mapping[state],
            styles=",\n\n".join(
                [self.c_prop_repr.format(
                    prop_name,
                    self.states[state][prop_name].c_repr())
                for prop_name in self.states[state]]))
            for state in self.states]

        return self.c_style_repr.format(
            type=self.type,
            styles=' | '.join([available_states[s] for s in self.states]),
            states=",\n".join(states))
