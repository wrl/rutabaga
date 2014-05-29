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

def find_freetype(conf, prefix, static=True, mandatory=True):
    params = {
        'stlib': 'freetype',
        'header_name': 'ft2build.h',
        'libpath': prefix + '/lib',
        'includes': [prefix + x for x in
            ['/include', '/include/freetype2']],
        'uselib_store': 'FREETYPE2',
        'mandatory': mandatory
    }

    if static:
        params['stlib'] = 'freetype'
    else:
        params['lib'] = 'freetype'

    return conf.check_cc(**params)

def check_freetype(conf):
    if conf.options.freetype_prefix:
        find_freetype(conf, conf.options.freetype_prefix, static=True)
    elif conf.env.DEST_OS == 'darwin':
        find_freetype(conf, conf.options.freetype_prefix or '/usr/local',
                static=False)
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

    tooldir = conf.path.find_dir('waftools').abspath()
    conf.load('objc', tooldir=tooldir)
    conf.load('rtb_style', tooldir=tooldir)
    conf.load('shader_header', tooldir=tooldir)

    # conf checks

    separator()

    if conf.env.DEST_OS == 'win32':
        check_freetype(conf)

        conf.env.append_unique('LIB_GL', ['opengl32', 'gdi32'])
        conf.define('WIN32_LEAN_AND_MEAN', 1)
        conf.env.PLATFORM = 'win'
    elif conf.env.DEST_OS == 'darwin':
        check_alloca(conf)
        check_freetype(conf)
        conf.env.PLATFORM = 'cocoa'

        conf.env.append_unique('FRAMEWORK_COCOA', ['Cocoa', 'QuartzCore'])
    else:
        check_alloca(conf)
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

    conf.env.RTB_VERSION = VERSION

    conf.define('_GNU_SOURCE', 1)
    conf.env.append_unique('CFLAGS', [
        '-std=gnu99', '-fms-extensions',
        '-Wall', '-Werror', '-Wextra', '-Wcast-align',
        '-Wno-microsoft', '-Wno-missing-field-initializers', '-Wno-unused-parameter',
        '-ffunction-sections', '-fdata-sections', '-ggdb'])

    if conf.options.debug_layout:
        conf.env.RTB_LAYOUT_DEBUG = True
        conf.define("RTB_LAYOUT_DEBUG", True)

    if conf.options.debug_frame:
        conf.define("_RTB_DEBUG_FRAME", True)

def build(bld):
    bld.recurse("styles")
    bld.recurse("third-party")
    bld.recurse("src")

    if bld.env.BUILD_EXAMPLES:
        bld.recurse("examples")
