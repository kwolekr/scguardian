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

#define CMD_BL		     0
#define CMD_BLACKLIST	 1
#define CMD_SB			 2
#define CMD_STATSBAN	 3
#define CMD_DLBAN		 4
#define CMD_IPBAN		 5
#define CMD_REPORTHACKER 6
#define CMD_PI			 7
#define CMD_PLAYERINFO	 8
#define CMD_RELOADDB	 9
#define CMD_UNLOAD		 10

#define CMD_BL_COUNTRY	 0x01
#define CMD_BL_IPADDR	 0x02
#define CMD_BL_SLOT		 0x04
#define CMD_BL_TAGBAN	 0x08


typedef struct _cmddesc {
	unsigned int hash;
	const char *str;
	//unsigned int flags;
} CMDDESC, *LPCMDDESC;


int GetCommandIndex(const char *text);
void __cdecl ParseLobbyCommand(int otherra, const char *text);
void HandleBlacklistCmd(char *args);
void HandlePlayerInfoCmd(char *args);

