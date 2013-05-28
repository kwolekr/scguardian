/*****************************************************************************
 *  _______ _______ _______  ______ _______  ______ _______ _______ _______	 *
 *  |______    |    |_____| |_____/ |       |_____/ |_____| |______    |     *
 *  ______|    |    |     | |    \_ |_____  |    \_ |     | |          |	 *
 *  																		 *
 *        ______ _     _ _______  ______ ______  _____ _______ __   _		 *
 *       |  ____ |     | |_____| |_____/ |     \   |   |_____| | \  |		 *
 *       |_____| |_____| |     | |    \_ |_____/ __|__ |     | |  \_|		 *
 *		    															     *
 *****************************************************************************/

/*-
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
 * main.c - 
 *    Contains DLL event handlers, main hooks, and other major functions
 */

#include "main.h"
#include "fxns.h"
#include "hook.h"
#include "commands.h"


HINSTANCE hInst;
HANDLE hThread;

FILE *fout;

HANDLE hEventShutdown;
HANDLE hEventGetCountry;
HANDLE hEventDoneRecving;

HINTERNET hInternet;

lpfnSNetDropPlayer SNetDropPlayer;
lpfnSNetSendMessage SNetSendMessage;

volatile long recvcallcount;

unsigned int gstate;

PLR players[8];

int gcqueue[8];
int numgcqueue;

//////////////////////////////////////////////////////////////////////////////////////


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
	HMODULE hStorm;
	if (fdwReason == DLL_PROCESS_ATTACH) {

		PrintText("Welcome to SC Guardian v" GUARD_VER "!");

		hInst = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL);

		hStorm = GetModuleHandle("storm.dll");
		if (!hStorm) {
			DispMsg("Failed to get base of Storm.dll");
			return 0;
		}
						
		SNetDropPlayer = (lpfnSNetDropPlayer)GetProcAddress(hStorm, (LPCSTR)106);
		if (!SNetDropPlayer) {
			DispMsg("Failed to find Storm!SNetDropPlayer");
			return 0;
		}

		SNetSendMessage = (lpfnSNetSendMessage)GetProcAddress(hStorm, (LPCSTR)127);
		if (!SNetSendMessage) {
			DispMsg("Failed to find Storm!SNetSendMessage");
			return 0;
		}

		HookFunction(0x47F8F0, (unsigned int)ParseLobbyCommand, 4);

		hEventDoneRecving = CreateEvent(NULL, 1, 1, NULL);
		hEventShutdown    = CreateEvent(NULL, 0, 0, NULL);
		hEventGetCountry  = CreateEvent(NULL, 0, 0, NULL);
		hThread		   	  = CreateThread(NULL, 0, DllProc, 0, 0, NULL);

		fout = fopen("scudp.log", "w");
		if (!fout)
			DispMsg("Error opening log file for write!");

		if (!InstallAPIHook("wsock32.dll", "recvfrom", "battle.snp", (int)recvfromhook))
			DispMsg("Error installing wsock32!recvfrom hook on battle.snp!");

		hInternet = InternetOpen("SC Guardian", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
		if (!hInternet)
			DispMsg("InternetOpen() failed!");

	} else if (fdwReason == DLL_PROCESS_DETACH) {
		PrintText("buh buy");

		InternetCloseHandle(hInternet);

		SetEvent(hEventShutdown);

		UnhookFunction(0x47F8F0, 4);
		
		fclose(fout);

		if (!RemoveAPIHook("wsock32.dll", "recvfrom", "battle.snp", (int)recvfromhook))
			DispMsg("Error removing wsock32!recvfrom hook!");
		
		WaitForSingleObject(hEventDoneRecving, INFINITE);	
	}
	return 1;
}


DWORD WINAPI DllProc(LPVOID lpParameter) {
	int event;
	unsigned int reqlen;
	char buf[256];
	char url[64];
	char tmp[4];
	HANDLE hEvents = {
		hEventShutdown,
		hEventGetCountry
	};

	while (1) {
		event = WaitForMultipleObjects(sizeof(hEvents) / sizeof(HANDLE), hEvents, 0, 150);
		switch (event) {
			case WAIT_TIMEOUT:
				if (GetKeyState(VK_F10) & 0xFF00) {
					PrintText("Hello there");
				} else if (GetKeyState(VK_F11) & 0xFF00) {
					FreeLibraryAndExitThread(hInst, 0);
					return 0;
				}
			case WAIT_OBJECT_0:
				return 0;
			case WAIT_OBJECT_1:     

				sprintf(url, "http://api.hostip.info/country.php?ip=%s", inet_ntoa(players[gcqueue[0]].name.sin_addr));
				reqlen = HttpGetUrl(url, tmp, 4);
				if (!reqlen || reqlen > 2)
					break;

				players[gcqueue[0]].country = *(short *)tmp;

				numgcqueue--;
				//////ermmm..... fix0r

				break;
			default:
				sprintf(buf, "Error %d at WaitForSingleObject() == %d!", GetLastError(), event);
				MessageBox(0, buf, 0, 0);
				return 0;
		}
	}
	return 0;
}


int recvfromhook(SOCKET s, char *buf, int len, int flags,
				 struct sockaddr *from, int *fromlen) {
	int retval, i, plr;
	LPSCUDPHDR udphdr;
	LPC0P6 pkt6info;

	//because it needs to be atomic!!
	InterlockedIncrement(&recvcallcount); 
	ResetEvent(hEventDoneRecving);

	retval = recvfrom(s, buf, len, flags, from, fromlen);

	InterlockedDecrement(&recvcallcount);
	if (!recvcallcount)
		SetEvent(hEventDoneRecving);

	if (!retval || retval == SOCKET_ERROR)
		return retval;

	for (i = 0; i != retval; i++)
		fprintf(fout, "%02x ", (unsigned char)buf[i]);
	fprintf(fout, "\r\n\r\n");

	if (retval >= sizeof(SCUDPHDR)) {
		udphdr = (LPSCUDPHDR)buf;
		switch(udphdr->packetclass) {
			case 0x00:
				switch (udphdr->packetid) {
					case 0x06:	
						pkt6info = (LPC0P6)(buf + sizeof(SCUDPHDR));
						
						plr = pkt6info->playerid;
						if (plr < 8 || plr >= 0) {
							players[plr].ishost = pkt6info->ishost;
							memcpy(&players[plr].name, &pkt6info->name, sizeof(struct sockaddr_in));
							strncpy(players[plr].username, (const char *)pkt6info + sizeof(C0P6), sizeof(players[plr].username));
							strncpy(players[plr].statstr, (const char *)pkt6info + sizeof(C0P6) + strlen(players[plr].username) + 1,
								sizeof(players[plr].statstr));
							GetCountryCode(plr);
						}
						break;
					case 0x07: //username and statstring
						AddLobbyText("\x03The user %s is joining with statstring \x03""%s (IP %s)",
							buf + 16, buf + 17 + strlen(buf + 16), 
							inet_ntoa(((struct sockaddr_in *)from)->sin_addr));
						break;
					case 0x0F:
						gstate |= GS_INLOBBY;
						break;
					default:
						; //AddLobbyText("\x03Got 0x%02x packet", (unsigned char)udphdr->packetid);
				}
			case 0x01:
				/*
				0x4F - Map data transfer
				  unsigned short (packet size [after value])
				  unsigned short (transfer ID)
					ID 0 - Unknown
						6 bytes
					ID 1 - File Name
						unsigned long (file size?)
						unsigned long (file hash?)
						string (file name)
					ID 4 - Transfer data
						unsigned char (unknown)
						unsigned long (file offset ?)
						unsigned short (block size [after value])
						void (..data..)
					ID 5 - Request data
						unsigned char (unknown)
						unsigned long (file offset ?) 
				*/
				switch (buf[0x10]) {
					case 0x4B: //unknown?
						break;
					case 0x4C: //chat
						//CheckPhraseban(data + 0x11);
						break;
				}
				break;
			case 0x02:
				switch (buf[0x10]) {
					case 0x3C: //game start
						//ingame = 1;
						break;
					case 0x3D: //downloading map
						//data[0x11] = map dl %
						if (gstate & GS_BANDOWNLOADERS) {
							AddLobbyText("Player %d is attempting to download, banning...", udphdr->playerid);
							BanUser(udphdr->playerid);
						}
						break;
					case 0x3E: //slot index
						//	data[0x11]; //the new player id
						//	data[0x12]; //the old player id
						//	data[0x13]; //slot status
						AddLobbyText("Player %d changed slot to %d (%d)", buf[0x12], buf[0x11], buf[0x13]);
						/*
						(BYTE) Slot Packet ID - 3E
						(BYTE) GameSlot ID
						(BYTE) Player ID*
						(BYTE) Slot Status**
						(BYTE) Player Race Selection
						(BYTE) Force***
						(BYTE) Unknown ID - 3F
						(BYTE) Player ID 
						(WORD) Null
						(WORD) 01 00
						(WORD) 05 00

						*
						When no player is there it is 0xFF

						**
						Not Used   0x00 (Meaning for example, you're in Lost Temple and there are only 4 slots allowed, the other 4 0x3E's won't even be used)
						Human      0x02
						Computer  0x05
						Empty       0x06
						Closed      0x08

						***
						No Force  0x00
						Force 1    0x01
						Force 2    0x02
						etc..
						*/
						break;
				}

		} 
	}

	return retval;
}


void GetCountryCode(int playerid) {
	int i, remove;

	if (numgcqueue < sizeof(gcqueue) / sizeof(int))	{
		remove = 0;
		for (i = 0; i != numgcqueue - 1; i++) {
			if (!remove) {
				if (gcqueue[i] == playerid)
					remove = 1;
			} else { 	
				gcqueue[i] = gcqueue[i + 1];
			}
		}
		gcqueue[numgcqueue] = playerid;
		numgcqueue++;
		SetEvent(hEventGetCountry);
	}
}


void BanUser(int playerid) {
	/*
	SNetSendMessage(playerid, "\x4E\x01", 2);
	Sleep(250);
	SNetSendMessage(playerid, "\x4E\x01", 2);
	Sleep(250);
	SNetDropPlayer(playerid, 3);
	*/ 
	__asm { //use starcraft's function instead
		mov esi, playerid
		mov eax, 1
		mov edx, 0x0470480
		call edx
	}

}


void PrintText(const char *fmt, ...) {
	char asdf[1024];
	va_list val;

	va_start(val, fmt);
	vsprintf(asdf, fmt, val);
	va_end(val);

	/*check for presence of 
	lobby chatbox BIN header struct*/
	if (*(LPBINHDR *)0x5999EC && (gstate & GS_INLOBBY)) 
		PrintInLobby(asdf);
	else
		PrintInBnet(asdf);
}


void PrintInBnet(const char *text) {
	HWND hwnd;

	if (IsBadReadPtr((const void *)0x1904610C, sizeof(HWND)))
		return;
	hwnd = *(HWND *)(0x1904610C);
				
	SendMessage(hwnd, 0x468, 0x12, (LPARAM)text);
}


void PrintInLobby(const char *text) {
	__asm {
		mov eax, text
		push 3
		mov edx, 0x004B8F10
		call edx
	}
}


void AddLobbyText(const char *fmt, ...) {
	char asdf[1024];
	va_list val;

	if (*(LPBINHDR *)0x5999EC && (gstate & GS_INLOBBY)) {
		va_start(val, fmt);
		vsprintf(asdf, fmt, val);
		va_end(val);

		__asm {
			lea eax, asdf
			push 3
			mov edx, 0x004B8F10
			call edx
		}
	}
}


void PrintInGame(const char *text) {
	__asm {
		xor eax, eax
		push 0
		mov edx, text
		push edx
		mov edx, 0x48D1C0
		call edx
	}
}

