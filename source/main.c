#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <3ds.h>

#include "archive.h"


typedef int (*menuent_funcptr)(void);

u8 *filebuffer;
size_t bufsize = 0x1200000;

int menu_dump();
int menu_configdump();
int menu_restore();

int mainmenu_totalentries = 3;
char *mainmenu_entries[3] = {
"Dump all extdata to sd card",
"Dump extdata specified in config",
"Restore extdata specified in config"};
menuent_funcptr mainmenu_entryhandlers[3] = {menu_dump, menu_configdump, menu_restore};

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
	backupAllExtdata(filebuffer, bufsize);
	
	gfxFlushBuffers();
	gfxSwapBuffers();
	return 0;
}

int menu_configdump()
{
	backupByConfig(filebuffer, bufsize);

	gfxFlushBuffers();
	gfxSwapBuffers();
	return 0;
}

int menu_restore()
{
	restoreFromSd(filebuffer, bufsize);

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

		ret = draw_menu(mainmenu_entries, mainmenu_totalentries, 1, 1);
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
	gfxInitDefault();

	consoleInit(GFX_BOTTOM, NULL);

	printf("Extdata dumper\n");
	gfxFlushBuffers();
	gfxSwapBuffers();

	consoleClear();
	
	filebuffer = (u8*)malloc(bufsize);
	if (filebuffer==NULL)
	{
		printf("malloc failed\n");
		gfxFlushBuffers();
		gfxSwapBuffers();
		svcSleepThread(5000000000LL);
	} else {
		handle_menus();
		free(filebuffer);
	}


	gfxExit();
	return 0;
}

