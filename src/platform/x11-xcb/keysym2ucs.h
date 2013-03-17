/*
 * This module converts keysym values into the corresponding ISO 10646-1
 * (UCS, Unicode) values.
 */

#include <wchar.h>
#include <X11/X.h>

wchar_t keysym2ucs(KeySym keysym);
