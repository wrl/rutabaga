let g:syntastic_mode_map = {
			\ "mode": "active",
			\ "active_filetypes": [],
			\ "passive_filetypes": []}

let g:syntastic_c_checkers = ['gcc']
let g:syntastic_c_compiler_options = "-std=c99 -fplan9-extensions -D_GNU_SOURCE"
let g:syntastic_c_include_dirs = [
			\ "include",
			\ "third-party",
			\ "third-party/gl3w",
			\ "build/src"]

set path+=include,third-party,third-party/gl3w,build/src,src
