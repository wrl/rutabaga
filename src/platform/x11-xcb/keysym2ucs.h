/*
 * This module converts keysym values into the corresponding ISO 10646-1
 * (UCS, Unicode) values.
 */

#include <uchar.h>
#include <X11/X.h>

char32_t keysym2ucs(KeySym keysym);
