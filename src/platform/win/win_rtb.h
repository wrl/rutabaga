/**
 * rutabaga: an OpenGL widget toolkit
 * Copyright (c) 2013-2018 William Light.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <windows.h>

#include <uv.h>

#include <rutabaga/rutabaga.h>
#include <rutabaga/window.h>

#ifndef _DPI_AWARENESS_CONTEXTS_
typedef HANDLE DPI_AWARENESS_CONTEXT;

#define DPI_AWARENESS_CONTEXT_UNAWARE              ((DPI_AWARENESS_CONTEXT) -1)
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE         ((DPI_AWARENESS_CONTEXT) -2)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    ((DPI_AWARENESS_CONTEXT) -3)

typedef enum DPI_AWARENESS {
	DPI_AWARENESS_INVALID,
	DPI_AWARENESS_UNAWARE,
	DPI_AWARENESS_SYSTEM_AWARE,
	DPI_AWARENESS_PER_MONITOR_AWARE
} DPI_AWARENESS;
#endif

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT) -4)
#endif

#ifndef DPI_ENUMS_DECLARED
typedef enum MONITOR_DPI_TYPE {
	MDT_EFFECTIVE_DPI = 0,
	MDT_ANGULAR_DPI   = 1,
	MDT_RAW_DPI       = 2,
	MDT_DEFAULT       = MDT_EFFECTIVE_DPI
} MONITOR_DPI_TYPE;
#endif

struct win_rtb {
	struct rutabaga rtb;

	HMODULE ole32;

	HCURSOR copy_cursor;

	DPI_AWARENESS_CONTEXT (*get_window_dpi_ctx)(HWND);
	DPI_AWARENESS_CONTEXT (*get_thread_dpi_ctx)();

	DPI_AWARENESS (*awareness_from_ctx)(DPI_AWARENESS_CONTEXT);
	UINT (*get_system_dpi)(void);
	UINT (*get_window_dpi)(HWND);
	HRESULT (*get_monitor_dpi)
		(HMONITOR, MONITOR_DPI_TYPE, UINT *dpi_x, UINT *dpi_y);
};

struct win_rtb_window {
	RTB_INHERIT(rtb_window);

	ATOM  window_class;
	HWND  hwnd;

	HGLRC gl_ctx;
	HDC   dc;

	volatile int event_loop_running;

	int capture_depth;
	int in_size_move;
};

LRESULT win_rtb_handle_message(struct win_rtb_window *,
		UINT message, WPARAM wparam, LPARAM lparam);
