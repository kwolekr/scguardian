/*
 * Copyright (c) 2010 Ryan Kwolek
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *  1. Redistributions of source code must retain the above copyright notice, this list of
 *     conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice, this list
 *     of conditions and the following disclaimer in the documentation and/or other materials
 *     provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* 
 * fxns.c - 
 *    Contains miscellaneous support functions and general use routines.
 */

#include "main.h"
#include "fxns.h"


//////////////////////////////////////////////////////////////////////////////////////////////


void lcase(char *str) {
	while (*str) {
		*str = tolower(*str);
		str++;
	}
}


unsigned int __fastcall hash(unsigned char *key) {
	unsigned int hash = 0;
    while (*key) {
		hash += *key++;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}


void DispMsg(const char *fmt, ...) {
	char asdf[1024];
	va_list val;

	va_start(val, fmt);
	vsprintf(asdf, fmt, val);
	va_end(val);

	MessageBox(NULL, asdf, NULL, 0);
}


int HttpGetUrl(const char *url, char *buf, int buflen) {
	int result;
	unsigned long bytesread;
	HINTERNET hUrl;

	if (!hInternet)
		return 0;

	hUrl = InternetOpenUrl(hInternet, url, NULL, 0, INTERNET_FLAG_NO_UI, 0);
	if (!hUrl)
		return 0;

	result = InternetReadFile(hUrl, buf, buflen, &bytesread);

	InternetCloseHandle(hUrl);
	return result ? bytesread : 0;
}

