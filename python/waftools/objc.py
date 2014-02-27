from waflib import TaskGen

@TaskGen.extension('.m')
def objc_hook(self, node):
	self.create_compiled_task('c', node)
