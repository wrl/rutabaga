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

sys.path.append("./python")

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

def check_freetype(conf):
	pkg_check(conf, "freetype2")

def check_x11(conf):
	check = lambda pkg: pkg_check(conf, pkg)

	check("x11-xcb")
	check("xcb")
	check("xcb-xkb")
	check("xcb-keysyms")
	check("xcb-icccm")
	check("xkbfile")
	check("xkbcommon")

def check_jack(conf):
	pkg_check(conf, "jack")

#
# waf stuff
#

def options(opt):
	opt.load('compiler_c')

	rtb_opts = opt.add_option_group("rutabaga options")
	rtb_opts.add_option("--debug-layout", action="store_true", default=False,
			help="when enabled, objects will draw their bounds in red.")

def configure(conf):
	separator()
	conf.load("compiler_c")
	conf.load("gnu_dirs")

	# conf checks

	separator()
	check_alloca(conf)
	separator()

	check_gl(conf)
	check_freetype(conf)

	check_x11(conf)

	separator()

	check_jack(conf)

	separator()

	# setting defines, etc

	conf.env.VERSION = VERSION
	conf.define("_GNU_SOURCE", "")
	conf.env.append_unique("CFLAGS", [
		"-std=c99", "-ggdb", "-Wall", "-Werror", "-Wno-microsoft", "-fms-extensions",
		"-ffunction-sections", "-fdata-sections"])

	if conf.options.debug_layout:
		conf.env.RTB_LAYOUT_DEBUG = True
		conf.define("RTB_LAYOUT_DEBUG", True)

def build(bld):
	bld.recurse("assets")
	bld.recurse("src")
	bld.recurse("examples")
