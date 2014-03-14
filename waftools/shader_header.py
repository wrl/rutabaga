#!/usr/bin/env python

from waflib.TaskGen import feature, extension, before_method
from waflib.Errors import WafError
from waflib import Task

from glsl2h import write_shader_header
 
@extension('glsl')
def glsl_file(self, *kw):
	pass

class glsl_to_c_header(Task.Task):
	color = 'CYAN'

	def run(self):
		write_shader_header(self.outputs[0], self.inputs)

@feature('shader_header')
@before_method('process_source')
def shader_header_feature(self):
	if not hasattr(self, 'vertex'):
		raise WafError('missing vertex shader')
	
	if not hasattr(self, 'fragment'):
		raise WafError('missing fragment shader')

	tgt = self.path.get_bld().find_or_declare(self.target)
	
	vert = self.path.get_src().find_node(self.vertex)
	frag = self.path.get_src().find_node(self.fragment)

	self.create_task('glsl_to_c_header', src=[vert, frag], tgt=tgt)

# vim: set ts=4 sts=4 sw=4 noet :
