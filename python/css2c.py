#!/usr/bin/env python

from __future__ import print_function

import sys
from collections import OrderedDict

from tinycss.parsing import ParseError, remove_whitespace
from tinycss_rutabaga import NamespaceRule, FontFaceRule, RutabagaCSSParser

all = [
    "RutabagaStyle",
    "RutabagaStylesheet",
    "ColorParseException"
]

def decl_dict(declarations):
    ret = {}

    for d in declarations:
        ret[d.name] = d.value

    return ret

class ColorParseException(Exception):
    pass

def parse_hex(h):
    if h[0] == '#':
        h = h[1:]

    if len(h) == 3:
        h = int(h, 16)
        r,g,b = [((h & (0xF << x)) >> x) / 15.0 for x in range(8, -1, -4)]
    elif len(h) == 6:
        h = int(h, 16)
        r,g,b = [((h & (0xFF << x)) >> x) / 255.0 for x in range(16, -1, -8)]
    else:
        raise ColorParseException()

    return (r,g,b)

def parse_rgba_func(args):
    args = [x for x in args if x.type in ['NUMBER', 'HASH']]

    if args[0].type == 'HASH':
        if len(args) != 2:
            raise ColorParseException()

        rgba = [0, 0, 0, args[1].value]
        rgba[:3] = parse_hex(args[0].value)
        return rgba

    else:
        if len(args) != 4:
            raise ColorParseException()

        return [x.value for x in args]

# XXX: hack
prop_name_mapping = {
    "color": "fg",
    "background": "bg"
}

class RutabagaRGBAProperty(object):
    def __init__(self, name, tokens):
        self.rgba = [0,0,0,1.0]
        self.name = name

        if tokens[0].type == 'HASH':
            try:
                self.rgba[:3] = parse_hex(tokens[0].value)
            except ColorParseException:
                raise ParseError(tokens[0], "couldn't parse hex color")

        elif tokens[0].type == 'FUNCTION' and tokens[0].function_name == 'rgba':
            try:
                self.rgba = parse_rgba_func(tokens[0].content)
            except ColorParseException:
                raise ParseError(
                    tokens[0], "function rgba() takes either 4 numeric"
                    "arguments or a hex color and alpha")

        else:
            raise ParseError(tokens[0], "expected hex color or rgba()")

    def c_repr(self):
        return '.{name} = {{{rgba[0]}, {rgba[1]}, {rgba[2]}, {rgba[3]}}}'.format(
                name=prop_name_mapping[self.name],
                rgba=self.rgba)


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

style_types = {
    "color":      RutabagaRGBAProperty,
    "background": RutabagaRGBAProperty
}


class RutabagaStyle(object):
    def __init__(self, type, normal_props):
        self.type = type
        self.states = {}

        self.add_state("normal", normal_props)

    def __repr__(self):
        return ('<{0.__class__.__name__} for {0.type}>'.format(self))

    def add_state(self, state, styles):
        self.states[state] = OrderedDict()

        for s in styles:
            try:
                self.states[state][s] = style_types[s](s, styles[s])
            except KeyError:
                raise ParseError(
                    styles[s][0], 'unknown property "{0}"'.format(s))

    c_state_repr = """\
\t\t\t[{state}] = {{
{styles}
\t\t\t}}"""

    c_style_repr = """\
\t{{"{type}",
\t\t{styles},

\t\t.states = {{
{states}
\t\t}}
\t}}"""

    def c_repr(self):
        states = [RutabagaStyle.c_state_repr.format(
            state=state_mapping[s],
            styles=",\n".join(
                ["\t\t\t\t" + self.states[s][sty].c_repr()
                    for sty in self.states[s]]))
            for s in self.states]

        return RutabagaStyle.c_style_repr.format(
            type=self.type,
            styles=' | '.join([available_states[s] for s in self.states]),
            states=",\n".join(states))


class RutabagaStylesheet(object):
    def __init__(self, css_file, autoparse=False):
        self.css_file = css_file
        self.parser = RutabagaCSSParser()

        self.styles = OrderedDict()
        self.namespaces = {}

        if autoparse:
            self.parse()

    def parse(self):
        self.in_namespace = None
        css = self.parser.parse_stylesheet(self.css_file.read())

        for rule in css.rules:
            if type(rule) == NamespaceRule:
                if rule.prefix:
                    self.namespaces[rule.prefix] = rule
                else:
                    self.in_namespace = rule

            elif type(rule) == FontFaceRule:
                pass

            else:
                sel = self.parse_selector(rule.selector)
                decls = decl_dict(rule.declarations)

                for s in sel:
                    if s.find(":") > 0:
                        s, state = s.split(":", 1)
                        if state not in state_mapping.keys():
                            raise ParseError(rule, 'unknown state "{0}"'.format(state))

                        self.styles[s].add_state(state, decls)
                    else:
                        self.styles[s] = RutabagaStyle(s, decls)

        if css.errors:
            for error in css.errors:
                print(error, file=sys.stderr)
                sys.exit(1)

    def parse_selector(self, selector):
        def pop():         return selector.pop() if selector else None
        def peek(n=1):     return selector[-n] if selector else None
        def is_comma(tok): return tok.type == 'DELIM' and tok.value == ','
        def bail(tok):
            raise ParseError(tok, 'unexpected "{0}"'.format(tok.type))

        ret = []

        while True:
            tok = pop()
            if not tok:
                return ret

            if is_comma(tok):
                continue

            if tok.type != "IDENT":
                bail(tok)

            ns = self.in_namespace
            val = tok.value

            while True:
                ptok = peek()

                if not ptok or is_comma(ptok):
                    if ns:
                        ret.append(ns.namespace + "." + val.lstrip())
                    else:
                        ret.append(val.lstrip())
                    break
                elif ptok.type == 'IDENT':
                    val = pop().value + val

                elif ptok.type == 'DELIM' and ptok.value == '|':
                    # namespace
                    pop()
                    ns = self.namespaces[pop().value]

                elif ptok.type == 'DELIM' and ptok.value == '.':
                    pop()
                    val = "." + val

                elif ptok.type == ':':
                    pop()
                    ptok = peek()

                    if ptok.type == ':':
                        # we treat `selector::pseudo` as being
                        # convenience syntax for `selector.pseudo`

                        pop()
                        ptok = peek()

                        if ptok.type != 'IDENT':
                            bail(ptok)

                        val = pop().value + "." + val
                    else:
                        val = ":" + val

                elif ptok.type == 'S':
                    pop()
                    val = " " + val

                else:
                    bail(ptok)

    c_repr_tpl = """\
static struct rtb_style {var_name}[] = {{
{style_structs}
}};"""

    def c_repr(self, var_name="default_style"):
        return RutabagaStylesheet.c_repr_tpl.format(
            var_name=var_name,
            style_structs=",\n\n".join(
                [self.styles[s].c_repr() for s in self.styles]
                    + ["\t{NULL}"]))


def main():
    css = RutabagaStylesheet(sys.stdin)
    css.parse()

    print(prelude)
    print(css.c_repr(), file=sys.stdout)

if __name__ == "__main__":
    main()
