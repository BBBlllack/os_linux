/*
 *  linux/lib/close.c
 *
 *  (C) 2025  shj
 */

#define __LIBRARY__
#include <unistd.h>

_syscall2(int, writeaddr, int, addr, int, val)
