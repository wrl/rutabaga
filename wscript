#!/usr/bin/env python

import subprocess
import time
import sys

top = "."
out = "build"

# change this stuff

APPNAME = "rutabaga"
VERSION = "0.1"

def separator():
    # just for output prettifying
    # print() (as a function) doesn't work in python <2.7
    sys.stdout.write("\n")

#
# dep checking functions
#

def check_alloca(conf):
    code = """
        #include <stdlib.h> /* *BSD have alloca in here */

        #ifdef NEED_ALLOCA_H
        #include <alloca.h>
        #endif

        int main(int argc, char **argv)
        {
            char *test = alloca(1);
            test[0] = 42;
            return 0;
        }"""

    if conf.check_cc(
        mandatory=False,
        execute=True,
        fragment=code,
        msg="Checking for alloca() in stdlib.h"):
        return

    if conf.check_cc(
        mandatory=True,
        execute=True,
        fragment=code,
        msg="Checking for alloca() in alloca.h",
        cflags=["-DNEED_ALLOCA_H=1"]):
        conf.define("NEED_ALLOCA_H", 1)

def pkg_check(conf, pkg):
    conf.check_cfg(
        package=pkg, args="--cflags --libs", uselib_store=pkg.upper())

def check_gl(conf):
    pkg_check(conf, "gl")

def find_freetype_static(conf, prefix):
    ft_prefix = conf.options.freetype_prefix

    conf.check_cc(stlib='freetype', header_name='ft2build.h',
        libpath=ft_prefix + '/lib',
        includes=[ft_prefix + x for x in
            ['/include', '/include/freetype2']],
        uselib_store='FREETYPE2')

def check_freetype(conf):
    if conf.env.DEST_OS in ['darwin', 'win32']:
        find_freetype_static(conf, conf.options.freetype_prefix or '/usr')
    elif conf.options.freetype_prefix:
        find_freetype_static(conf, conf.options.freetype_prefix)
    else:
        pkg_check(conf, "freetype2")

def check_x11(conf):
    check = lambda pkg: pkg_check(conf, pkg)

    check("x11-xcb")
    check("xcb")
    check("xcb-xkb")
    check("xcb-icccm")
    check("xkbfile")
    check("xkbcommon")

def check_jack(conf):
    if conf.env.DEST_OS in ['darwin', 'win32']:
        conf.check_cc(lib='jack', uselib_store='JACK', mandatory=False)
    else:
        pkg_check(conf, "jack")

def check_submodules(conf):
    if not conf.path.find_resource('third-party/libuv/uv.gyp'):
        raise conf.errors.ConfigurationError(
                "Submodules aren't initialized!\n"
                "Make sure you've done `git submodule init && git submodule update`.")

#
# waf stuff
#

def options(opt):
    opt.load('compiler_c')

    rtb_opts = opt.add_option_group("rutabaga options")
    rtb_opts.add_option("--debug-layout", action="store_true", default=False,
            help="when enabled, objects will draw their bounds in red.")
    rtb_opts.add_option("--debug-frame", action="store_true", default=False,
            help="when enabled, the rendering time for each frame (as "
                 "reported by openGL) will be printed to stdout")
    rtb_opts.add_option('--freetype-prefix', action='store', default=False,
            help='specify the path to the freetype2 installation')

def configure(conf):
    separator()
    check_submodules(conf)

    conf.load("compiler_c")
    conf.load("gnu_dirs")

    conf.load('objc', tooldir='./waftools')
    conf.load('rtb_style', tooldir='./waftools')
    conf.load('shader_header', tooldir='./waftools')

    # conf checks

    separator()
    check_alloca(conf)
    separator()

    if conf.env.DEST_OS == 'win32':
        pass
    elif conf.env.DEST_OS == 'darwin':
        check_freetype(conf)
        conf.env.PLATFORM = 'cocoa'

        conf.env.append_unique('FRAMEWORK_COCOA', ['Cocoa', 'QuartzCore'])
    else:
        check_gl(conf)
        check_freetype(conf)
        check_x11(conf)

        conf.env.PLATFORM = 'x11-xcb'
        separator()

    # if rutabaga is included as part of another project and this configure()
    # is running because a wscript up the tree called it, we don't build
    # the example projects.
    if not conf.stack_path[-1]:
        conf.env.BUILD_EXAMPLES = True
        check_jack(conf)
        separator()
    else:
        conf.env.BUILD_EXAMPLES = False

    # setting defines, etc

    conf.env.VERSION = VERSION
    conf.define('_GNU_SOURCE', '')
    conf.env.append_unique('CFLAGS', [
        '-std=c99', '-fms-extensions',
        '-Wall', '-Werror', '-Wextra', '-Wno-cast-align',
        '-Wno-microsoft', '-Wno-missing-field-initializers', '-Wno-unused-parameter',
        '-ffunction-sections', '-fdata-sections', '-ggdb'])

    if conf.options.debug_layout:
        conf.env.RTB_LAYOUT_DEBUG = True
        conf.define("RTB_LAYOUT_DEBUG", True)

    if conf.options.debug_frame:
        conf.define("_RTB_DEBUG_FRAME", True)

def build(bld):
    bld.recurse("styles")
    bld.recurse("src")
    bld.recurse("third-party")

    if bld.env.BUILD_EXAMPLES:
        bld.recurse("examples")
