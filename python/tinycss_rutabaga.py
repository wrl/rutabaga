# coding: utf8

from tinycss import *
from tinycss.parsing import ParseError, remove_whitespace

all = ['NamespaceRule', 'FontFaceRule', 'RutabagaCSSParser']


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
