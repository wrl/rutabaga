#!/usr/bin/env python

from tinycss.parsing import ParseError, remove_whitespace
from tinycss_rutabaga import NamespaceRule, FontFaceRule, RutabagaCSSParser

namespaces = {}
in_namespace = None

def parse_selector(selector):
    def pop():     return selector.pop() if selector else None
    def peek(n=1): return selector[-n] if selector else None
    def bail(tok): raise ParseError(tok, 'unexpected "{0}"'.format(tok.type))
    def is_comma(tok): return tok.type == 'DELIM' and tok.value == ','

    ret = []

    while True:
        tok = pop()
        if not tok:
            return ret

        if is_comma(tok):
            continue

        if tok.type != "IDENT":
            bail(tok)

        ns = in_namespace
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
                ns = namespaces[pop().value]

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

def main():
    global namespaces, in_namespace

    parser = RutabagaCSSParser()

    with open("assets/style.css") as f:
        css = parser.parse_stylesheet(f.read())

    print("")
    print("---")
    print("")

    for rule in css.rules:
        if type(rule) == NamespaceRule:
            if rule.prefix:
                namespaces[rule.prefix] = rule
            else:
                in_namespace = rule

        elif type(rule) == FontFaceRule:
            pass

        else:
            s = parse_selector(rule.selector)
            print(s)

            for d in rule.declarations:
                print("    {0}".format(d))

    print("")
    print("---")
    print("")

    for error in css.errors:
        print(error)

if __name__ == "__main__":
    main()
