/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <stdarg.h>
#include <libc/file.h>

int
fscanf(FILE *f, const char *fmt, ...)
{
  int r;
  va_list a=0;
  va_start(a, fmt);
  r = _doscan(f, fmt, a);
  va_end(a);
  return r;
}

int
fwscanf(FILE *f, const wchar_t *fmt, ...)
{
  int r;
  va_list a=0;
  va_start(a, fmt);
  r = _dowscan(f, fmt,(void *) a);
  va_end(a);
  return r;
}
