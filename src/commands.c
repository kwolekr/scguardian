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
 * commands.c - 
 *    Contains user input parsing functions and routines.
 */

#include "main.h"
#include "fxns.h"
#include "commands.h"

CMDDESC cmds[] = {
	{0x76cbfeff, "bl"		     },
	{0x1d106c54, "blacklist"	 },
	{0xf2336334, "sb"			 },
	{0xda989c72, "statsban"		 },
	{0x873e3ae7, "dlban"		 },
	{0xed8a4709, "ipban"	     },
	{0x67be956f, "reporthacker"	 },
	{0xd319a3ed, "pi"			 },
	{0xdf1e166d, "playerinfo"	 },
	{0x501cf663, "reloaddb"		 },
	{0x8ec52212, "unload"		 }
};


//////////////////////////////////////////////////////////////////////////////////////////////////


int GetCommandIndex(const char *text) {
	int i;
	unsigned int hashval;
	
	hashval = hash((unsigned char *)text);

	for (i = 0; i != sizeof(cmds) / sizeof(CMDDESC); i++) {
		if ((cmds[i].hash == hashval) &&
			(!strcmp(cmds[i].str, text))) {
			return i;
		}
	}
	return -1;
}


void __cdecl ParseLobbyCommand(int otherra, const char *text) {
	int cmdindex;
	char *cmd, *args;
	const char *tmptxt;

	//PrintText("look - %s !", text);

	tmptxt = text;

	if (*tmptxt++ != '/')
		return;
	if (!*tmptxt)
		return;

	cmd = strdup(tmptxt);

	args = strchr(cmd, ' ');
	if (args)
		*args++ = 0;

	lcase(cmd);

	cmdindex = GetCommandIndex(cmd);
	if (cmdindex == -1) {
		free(cmd);
		return;
	}

	switch (cmdindex) {
		case CMD_BL:
		case CMD_BLACKLIST:
			HandleBlacklistCmd(args);
			break;
		case CMD_SB:
		case CMD_STATSBAN:

			break;
		case CMD_DLBAN:

			break;
		case CMD_IPBAN:

			break;
		case CMD_REPORTHACKER:

			break;
		case CMD_PI:
		case CMD_PLAYERINFO:
			HandlePlayerInfoCmd(args);
			break;
		case CMD_RELOADDB:

			break;
		case CMD_UNLOAD:
			; //shutdown
	}
	free(cmd);
}


void HandleBlacklistCmd(char *args) {
	/*
	  /blacklist [/flags] [str]	  
		Flags:
		cC - country
		iI - ip address	
		sS - ban by slot
		tT - tagban

		No arguments will enumerate all the blacklisted entities.
		Only the flags argument will enumerate all the blacklisted entities applying to the specified flags.
		Only the str argument will blacklist the specified player name.
		Both the flags argument and the str argument will do its specifically defined job --
	*/
	int flags;
	char *blah;

	flags = 0;
	blah  = NULL;

	if (args) {		
		if (*args == '/') {
			do {
				args++;
				switch (*args) {
					case 'c':
					case 'C':
						flags |= CMD_BL_COUNTRY;
						break;
					case 'i':
					case 'I':
						flags |= CMD_BL_IPADDR;
						break;
					case 's':
					case 'S':
						flags |= CMD_BL_SLOT;
						break;
					case 't':
					case 'T':
						flags |= CMD_BL_TAGBAN;
				}
			} while (*args > ' ');

			if (*args == ' ') {
				blah = args;
				*blah++ = 0;
			} else if (!*args) {
				goto enumbl;
			} else {
				return;
			}
		} else {
			blah = args;
		}

		if (flags & CMD_BL_COUNTRY) {
			//...
		} else if (flags & CMD_BL_IPADDR) {
			//TODO: check for cidr notation in tagbans	
		}
	} else {
enumbl:
		; //TODO: enumerate all here
	}
}


void HandlePlayerInfoCmd(char *args) {
	int i;
	for (i = 0; i != 8; i++) {
		if (!stricmp(players[i].username, args))
			goto found;
	}
	return;
found:
	AddLobbyText("\x02""%s: %s:%d%s, statstr: %s",
		players[i].username, inet_ntoa(players[i].name.sin_addr),
		htons(players[i].name.sin_port), players[i].ishost
		 ? " (host)" : "", players[i].statstr);	
	
}

