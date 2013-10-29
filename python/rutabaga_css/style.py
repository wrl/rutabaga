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
from rutabaga_css.properties.font import *

all = ["RutabagaStyle"]

state_mapping = {
    "normal": "RTB_DRAW_NORMAL",
    "focus":  "RTB_DRAW_FOCUS",
    "hover":  "RTB_DRAW_HOVER",
    "active": "RTB_DRAW_ACTIVE"
}

prop_mapping = {
    "color": RutabagaRGBAProperty,
    "background-color": RutabagaRGBAProperty,
    "background-image": RutabagaTextureProperty,
    "border-color": RutabagaRGBAProperty
}

class RutabagaStyleState(object):
    def __init__(self, stylesheet):
        self.stylesheet = stylesheet
        self.props = OrderedDict()

        self.font_descriptor = {
            'family': None,
            'weight': None,
            'size':   None,
            'gamma':  2.2}

    def parse_font_tokens(self, prop, tokens):
        if prop == 'font-family':
            self.font_descriptor['family'] = tokens[0].value
        elif prop == 'font-size':
            # XXX: disregarding unit
            self.font_descriptor['size'] = tokens[0].value
        elif prop == '-rtb-font-lcd-gamma':
            self.font_descriptor['gamma'] = tokens[0].value

    def add_prop(self, prop, tokens):
        if prop in ('font-family', 'font-weight',
                'font-size', '-rtb-font-lcd-gamma'):
            self.parse_font_tokens(prop, tokens)
            return

        try:
            self.props[prop] = \
                    prop_mapping[prop](self.stylesheet, prop, tokens)
        except KeyError:
            raise ParseError(tokens[0],
                        'unknown property "{0}"'.format(prop))

    c_state_repr = """\
\t\t\t[{state}] = (struct rtb_style_property_definition []) {{
{properties}
\t\t\t}}"""

    c_empty_state_repr = """\
\t\t\t[{state}] = (struct rtb_style_property_definition []) {{{{NULL}}}}"""

    c_prop_repr = """\
\t\t\t\t{{"{0}",
{1}}}"""

    def done_parsing(self):
        if self.font_descriptor['family']:
            self.props['font'] = RutabagaFontProperty(self.stylesheet,
                    'font', **self.font_descriptor)

    def c_repr(self, state_name):
        if not self.props:
            return self.c_empty_state_repr.format(
                    state=state_mapping[state_name])

        return self.c_state_repr.format(
            state=state_mapping[state_name],
            properties=",\n\n".join(
                [self.c_prop_repr.format(
                    prop_name, self.props[prop_name].c_repr())
                    for prop_name in self.props]
                + ["\t\t\t\t{NULL}"]))

class RutabagaStyle(object):
    def __init__(self, stylesheet, type, normal_props):
        self.stylesheet = stylesheet

        self.type = type
        self.states = OrderedDict()

        for s in ("normal", "focus", "hover", "active"):
            self.states[s] = RutabagaStyleState(stylesheet)

        self.add_state("normal", normal_props)

    def __repr__(self):
        return ('<{0.__class__.__name__} for {0.type}>'.format(self))

    def add_state(self, state, props):
        if state not in state_mapping.keys():
            raise AttributeError('"{0}" is not a valid state'.format(state))

        for prop in props:
            self.states[state].add_prop(prop, props[prop])

    def done_parsing(self):
        for s in self.states:
            self.states[s].done_parsing()

    c_style_repr = """\
\t{{"{type}",
\t\t.inherit_from = NULL,
\t\t.resolved_type = NULL,
\t\t.properties = {{
{state_definitions}
\t\t}}
\t}}"""

    def c_repr(self):
        return self.c_style_repr.format(
            type=self.type,
            state_definitions=",\n".join(
                [self.states[state].c_repr(state)
                    for state in self.states]))
