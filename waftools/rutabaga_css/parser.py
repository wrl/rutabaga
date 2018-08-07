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

# coding: utf8

from .tinycss import *
from .tinycss.parsing import ParseError, remove_whitespace

all = [
    "ParseError",
    "NamespaceRule",
    "FontFaceRule",
    "RutabagaCSSParser"]

class NamespaceRule(object):
    at_keyword = '@namespace'

    def __init__(self, namespace, prefix, line, column):
        self.namespace = namespace
        self.prefix = prefix or ""
        self.line = line
        self.column = column

    def __repr__(self):
        return ('<{0.__class__.__name__} {0.line}:{0.column} {0.namespace}>'
                .format(self))


class FontFaceRule(object):
    at_keyword = '@font-face'

    def __init__(self, declarations, line, column):
        self.declarations = declarations
        self.line = line
        self.column = column

    def __repr__(self):
        return ('<{0.__class__.__name__} {0.line}:{0.column}>'
                .format(self))


class RutabagaCSSParser(CSS21Parser):
    def parse_at_rule(self, rule, previous_rules, errors, context):
        if rule.at_keyword == '@namespace':
            if context != 'stylesheet':
                raise ParseError(
                    rule, '@namespace rule not allowed in ' + context)
            if not rule.head:
                raise ParseError(rule, 'expected namespace for @namespace')

            ns, prefix = self.parse_namespace(rule.head)
            return NamespaceRule(ns, prefix, rule.line, rule.column)

        elif rule.at_keyword == '@font-face':
            declarations, at_rules, rule_errors = \
                self.parse_declarations_and_at_rules(rule.body, '@font-face')
            return FontFaceRule(declarations, rule.line, rule.column)

        else:
            return super().parse_at_rule(
                rule, previous_rules, errors, context)

    def parse_namespace(self, tokens):
        ns, prefix = None, None
        tokens = remove_whitespace(tokens)

        if tokens[0].type == "IDENT":
            prefix = tokens.pop(0).value
        ns = tokens.pop(0).value

        if tokens:
            raise ParseError(tokens[0], 'extraneous arguments to @namespace')

        return (ns, prefix)
