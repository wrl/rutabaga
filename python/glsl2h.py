#!/usr/bin/env python

def write_shader_header(outfile, infiles):
	output = ""

	shname = outfile.name
	shname = shname[:shname.find(".glsl.h")].replace("-", "_").upper()

	for file in infiles:
		if file.name.find("vert") > 0:
			shtype = "VERT"
		elif file.name.find("frag") > 0:
			shtype = "FRAG"
		else:
			raise Exception("i have no idea what kind of shader \"{0}\" is.".format(file.name))

		output += "static const char *{0}_{1}_SHADER = ".format(shname, shtype)

		f = open(file.abspath())
		for line in f:
			line = line.rstrip()

			if len(line) == 0:
				output += "\n"
			else:
				output += "\n\t\"{0}\\n\"".format(line.replace("\"", '\\\"'))

		output += ";\n\n"

	# get rid of the last newline
	output = output[:len(output) - 1]
	outfile.write(output)
