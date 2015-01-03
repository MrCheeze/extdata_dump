#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <3ds.h>

#include "archive.h"


typedef int (*menuent_funcptr)(void);

u8 *filebuffer;
size_t bufsize = 0x1000000;

int menu_dump();
int menu_restore();

int mainmenu_totalentries = 2;
char *mainmenu_entries[2] = {
"Dump extdata to sd card",
"Restore extdata from SD card, based on ini file"};
menuent_funcptr mainmenu_entryhandlers[2] = {menu_dump, menu_restore};

int draw_menu(char **menu_entries, int total_menuentries, int x, int y)
{
	int i;
	int cursor = 0;
	int update_menu = 1;
	int entermenu = 0;

	while(aptMainLoop())
	{
		gspWaitForVBlank();
		hidScanInput();

		u32 kDown = hidKeysDown();

		if(kDown & KEY_A)
		{
			entermenu = 1;
			break;
		}
		if(kDown & KEY_B)return -1;

		if(kDown & KEY_UP)
		{
			update_menu = 1;
			cursor--;
			if(cursor<0)cursor = total_menuentries-1;
		}

		if(kDown & KEY_DOWN)
		{
			update_menu = 1;
			cursor++;
			if(cursor>=total_menuentries)cursor = 0;
		}

		if(update_menu)
		{
			for(i=0; i<total_menuentries; i++)
			{
				if(cursor!=i)printf("\x1b[%d;%dH   %s", y+i, x, menu_entries[i]);
				if(cursor==i)printf("\x1b[%d;%dH-> %s", y+i, x, menu_entries[i]);
			}

			gfxFlushBuffers();
			gfxSwapBuffers();
		}
	}

	if(!entermenu)return -2;
	return cursor;
}

int menu_dump()
{
	backupAllExtdata(filebuffer);

	gfxFlushBuffers();
	gfxSwapBuffers();
	return 0;
}

int menu_restore()
{
	restoreFromSd(filebuffer);

	gfxFlushBuffers();
	gfxSwapBuffers();
	return 0;
}

int handle_menus()
{
	int ret;

	gfxFlushBuffers();
	gfxSwapBuffers();

	while(aptMainLoop())
	{
		consoleClear();

		ret = draw_menu(mainmenu_entries, mainmenu_totalentries, 0, 0);
		consoleClear();

		if(ret<0)return ret;

		ret = mainmenu_entryhandlers[ret]();
		if(ret==-2)return ret;

		svcSleepThread(5000000000LL);
	}

	return -2;
}

int main()
{

	// Initialize services
	gfxInit();

	consoleInit(GFX_BOTTOM, NULL);

	printf("Extdata dumper\n");
	gfxFlushBuffers();
	gfxSwapBuffers();

	consoleClear();
	
	filebuffer = (u8*)malloc(bufsize);
	handle_menus();
	free(filebuffer);


	gfxExit();
	return 0;
}

