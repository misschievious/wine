/*
 * Unicode string management
 *
 * Copyright 1995 Martin von Loewis
 *
 */

#ifndef _STRING32_H
#define _STRING32_H

#include "wintypes.h"

void	STRING32_UniToAnsi(LPSTR dest,LPCWSTR src);
void	STRING32_AnsiToUni(LPWSTR dest,LPCSTR src);
LPSTR	STRING32_DupUniToAnsi(LPCWSTR src);
LPWSTR	STRING32_DupAnsiToUni(LPCSTR src);
LPWSTR	STRING32_strdupW(LPCWSTR);

#endif
