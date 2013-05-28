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

#define _WIN32_WINNT 0x501
#define WIN32_LEAN_AND_MEAN

#include <windows.h> 
#include <stdlib.h>
#include <stdio.h>
#include <winsock2.h>
#include <dbghelp.h>
#include <wininet.h>

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "DbgHelp.lib")
#pragma comment(lib, "Wininet.lib")

#pragma optimize("gsy", on)
#pragma comment(linker,"/RELEASE")
#pragma comment(linker,"/ignore:4078")
#pragma comment(linker,"/FILEALIGN:0x200")
#pragma comment(linker,"/opt:nowin98")
#pragma comment(linker,"/ALIGN:4096")

#define GUARD_VER "0.3"

#define GS_BANDOWNLOADERS   0x01
#define GS_IPBAN			0x02
#define GS_INLOBBY			0x04
#define GS_INGAME			0x08

#define BNETTEXT_CLR_WHITE  0x17
#define BNETTEXT_CLR_GRAY   0x11
#define BNETTEXT_CLR_AQUA   0x14
#define BNETTEXT_CLR_RED    0x19
#define BNETTEXT_CLR_YELLOW	0x13
#define BNETTEXT_CLR_GREEN  0x12

#define CTL_DIALOGBOX	   0
#define CTL_DEFAULTBUTTON  1
#define CTL_BUTTON		   2
#define CTL_OPTIONBUTTON   3
#define CTL_CHECKBOX	   4
#define CTL_IMAGE		   5
#define CTL_HORSCROLLBAR   6
#define CTL_VERTSCROLLBAR  7
#define CTL_TEXTBOX		   8
#define CTL_LABELLEFT	   9
#define CTL_LABELRIGHT	   10
#define CTL_LABELCENTER	   11
#define CTL_LISTBOX		   12
#define CTL_COMBOBOX	   13
#define CTL_LIGHTUPBUTTON  14

#define SMK_FADEIN	 0x01
#define SMK_DARK	 0x02
#define SMK_REPEAT	 0x04
#define SMK_SHOWOVER 0x08
#define SMK_RESERVED 0x10


typedef BOOL (__stdcall *lpfnSNetDropPlayer)(int playerid, DWORD flags);
typedef BOOL (__stdcall *lpfnSNetSendMessage)(int playerid, char *data, int len);


typedef struct _scudphdr {
	int null;
	unsigned short chksum;
	unsigned short packetlen;
	unsigned short seqsend;
	unsigned short seqrecv;
	char packetclass;
	char packetid;
	char playerid;
	char resend;
} SCUDPHDR, *LPSCUDPHDR;

typedef struct _smk {
	unsigned long smkoverlay;
	unsigned short flags;
	unsigned long reserved1;
	char *filename;
	unsigned long reserved2;
	unsigned short overlayx;
	unsigned short overlayy;
	unsigned long reserved3;
} SMK, *LPSMK;

typedef struct _binhdr {
	struct _binhdr *nextbin;
	SMALL_RECT rc;
	unsigned short width;
	unsigned short height;
	unsigned long rtflags;
	char **strings;
	unsigned long wndflags;
	unsigned long reserved1;
	unsigned short controlid;
	unsigned long controltype;
	unsigned long reserved2;
	unsigned long updatefn1;
	unsigned long updatefn2;
	HWND parentwnd;
	SMALL_RECT responserect;
	unsigned long reserved3;
	LPSMK smk;
	unsigned short xtextindex;
	unsigned short ytextindex;
	unsigned short responsewidth;
	unsigned short responseheight;
	unsigned long reserved4;
	unsigned long reserved5;
} BINHDR, *LPBINHDR;

typedef struct _plr {
	char username[32];
	char statstr[32];	 //64
	int ishost;			 //68
	unsigned short country;	 //70
	struct sockaddr_in name; //86

	//128 - 86 = 42
	char padding[42];
} PLR, *LPPLR;

typedef struct _c0p6 {
	unsigned int len;
	int playerid;
	int ishost;
	int reserved1;
	int recved;
	struct sockaddr_in name;
} C0P6, *LPC0P6;


extern PLR players[8];

extern HINTERNET hInternet;	 


unsigned long __stdcall DllProc(void *param);
void BanUser(int playerid);
int recvfromhook(SOCKET s, char *buf, int len, int flags,
				 struct sockaddr *from, int *fromlen);

void PrintText(const char *fmt, ...);
void PrintInBnet(const char *text);
void PrintInLobby(const char *text);
void AddLobbyText(const char *fmt, ...);
void PrintInGame(const char *text);

