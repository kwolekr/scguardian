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
 * hook.c - 
 *    Contains routines for hooking and unhooking functions via the trampoline method,
 *    or replacing an IAT entry for imported functions needing hooks.
 */

#include "main.h"
#include "fxns.h"
#include "hook.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////


/*
 NB: Functions hooked with HookFunction have the obligations of:
		- Using the __cdecl calling convention
		- Preserving any registers being used as parameters
		- Be reentrant and multithread safe	````
		- Have the first stack parameter act as a 'dummy' for the previous RA
  
*/	
void HookFunction(unsigned int origfunc, unsigned int hookfn, int padlen) {	
	unsigned long oldprotect;
	int i;
	char *blah;
	/*				//size		  //offset
		call MyFunc	//5			 | 0
		DoOrigStuff	//5 + padlen | 5
		jmp OrigFxn	//5			 | 10 + padlen
	*/
	blah = (char *)VirtualAlloc(NULL, 15 + padlen, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	*blah = (char)0xE8;
	*(int *)(blah + 1) = ((hookfn - (unsigned int)blah) - 5);
	
	memcpy(blah + 5, (const void *)origfunc, 5 + padlen);

	blah[10 + padlen] = (char)0xE9;
	*(int *)(blah + 11 + padlen) = (origfunc + 5) - ((unsigned int)blah + 11 + padlen);
	
	VirtualProtect((void *)origfunc, 5 + padlen, PAGE_EXECUTE_READWRITE, &oldprotect);
	*(char *)origfunc = (char)0xE9;
	*(int *)(origfunc + 1) = (int)(blah - origfunc - 5);
	for (i = 0; i != padlen; i++)
		((char *)origfunc)[5 + i] = (char)0x90;
	VirtualProtect((void *)origfunc, 5 + padlen, oldprotect, &oldprotect);
}


void UnhookFunction(unsigned int func, int padlen) {
	unsigned long oldprotect;
	void *interm;

	interm = (void *)(*(int *)(func + 1) + func + 5);

	VirtualProtect((void *)func, 5 + padlen, PAGE_EXECUTE_READWRITE, &oldprotect);
	memcpy((void *)func, (char *)interm + 5, 5 + padlen);
	VirtualProtect((void *)func, 5 + padlen, oldprotect, &oldprotect);

	VirtualFree(interm, 15 + padlen, MEM_RELEASE);
}


int InstallAPIHook(const char *modname, const char *fnname,
				   const char *targetmod, int newfn) {
	int len, i, *blah, lookingfor;
	unsigned long oldprotect;

	HANDLE hModule = GetModuleHandle(targetmod);
	if (!hModule) {
		DispMsg("Failed to get base of %s!", targetmod);
		return 0;
	}

	blah = (int *)ImageDirectoryEntryToData((void *)hModule,
		1, IMAGE_DIRECTORY_ENTRY_IAT, (unsigned long *)&len);
	lookingfor = (int)GetProcAddress(GetModuleHandle(modname), fnname);

	for (i = 0; i != len; i++) {
		if (lookingfor == blah[i])
			goto success;
	}

	DispMsg("Failed to find %s in IAT of %s!", fnname, modname);
	return 0;

success:
	VirtualProtect(&blah[i], sizeof(int), PAGE_EXECUTE_READWRITE, &oldprotect);
	blah[i] = newfn;
	VirtualProtect(&blah[i], sizeof(int), oldprotect, &oldprotect);
	return 1;
}


int RemoveAPIHook(const char *modname, const char *fnname,
				  const char *targetmod, int hookfn) {
	int len, i, *blah;
	unsigned long oldprotect;

	HANDLE hModule = GetModuleHandle(targetmod);
	if (!hModule) {
		DispMsg("Failed to get base of %s!", targetmod);
		return 0;
	}

	blah = (int *)ImageDirectoryEntryToData((void *)hModule,
		1, IMAGE_DIRECTORY_ENTRY_IAT, (unsigned long *)&len);

	for (i = 0; i != len >> 2; i++) {
		if (hookfn == blah[i])
			goto success;
	}

	DispMsg("Failed to find %s in IAT of %s!", fnname, modname);
	return 0;

success:
	VirtualProtect(&blah[i], sizeof(int), PAGE_EXECUTE_READWRITE, &oldprotect);
	blah[i] = (int)GetProcAddress(GetModuleHandle(modname), fnname);
	VirtualProtect(&blah[i], sizeof(int), oldprotect, &oldprotect);
	return 1;
}


void __declspec(naked) SkipOrigFunction(int numparams) {
	__asm {
		ret            //esp += 4
		mov esp, ebp
		pop ebp		  
		ret 		  //esp += 4
		ret   //esp += 4 + numparams * 4

	}
}

