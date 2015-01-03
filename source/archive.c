#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <3ds.h>
#include <stdlib.h>

#include "archive.h"

FS_archive extdata_archive;
size_t q = 0x1000000;
u8 *filebuffer;

void unicodeToChar(char* dst, u16* src) {
	if(!src || !dst)return;
	while(*src)*(dst++)=(*(src++))&0xFF;
	*dst=0x00;
}

void foo(char *path, u32 lowpath_id, char *dumpfolder) {
	Handle extdata_dir;
	Result ret = FSUSER_OpenDirectory(NULL, &extdata_dir, extdata_archive, FS_makePath(PATH_CHAR, path));
	if (ret!=0) {
		printf("could not open dir\n");
		gfxFlushBuffers();
		gfxSwapBuffers();
		return;
	}
	char string4[0x120];
	sprintf(string4, "%s/%08x%s", dumpfolder, (unsigned int) lowpath_id, path);
	mkdir(string4, 0777);
	FS_dirent dirStruct;
	char fileName[0x106] = "";
	int cont = 0;
	while(1) {
		u32 dataRead = 0;
		FSDIR_Read(extdata_dir, &dataRead, 1, &dirStruct);
		if(dataRead == 0) break;
		unicodeToChar(fileName, dirStruct.name);
		printf("name: %s%s%s\n", path, fileName, dirStruct.isDirectory ? " (DIRECTORY)" : "");
		gfxFlushBuffers();
		gfxSwapBuffers();
		cont++;
		if (dirStruct.isDirectory) {
			char newpath[0x120];
			sprintf(newpath, "%s%s/", path, fileName);
			foo(newpath, lowpath_id, dumpfolder);
		} else {
			char string1[0x120];
			char string2[0x120];
			char string3[0x120];

			sprintf(string1, "%s%s", path, fileName);
			sprintf(string2, "%s/%08x%s%s", dumpfolder, (unsigned int) lowpath_id, path, fileName);
			sprintf(string3, "%08x%s%s", (unsigned int) lowpath_id, path, fileName);
			archive_copyfile(Extdata_Archive, SDArchive, string1, string2, filebuffer, 0, q, string3);
		}
	}
	printf("total files in 0x%08x%s: %d\n", (unsigned int) lowpath_id, path, (unsigned int) cont);
	gfxFlushBuffers();
	gfxSwapBuffers();
	FSDIR_Close(extdata_dir);
}

void getMName(int i, char *buffer) {
	strcpy(buffer, "UNKNOWN");
	if (i==0) strcpy(buffer, "NAND");
	if (i==1) strcpy(buffer, "SDMC");
	if (i==2) strcpy(buffer, "GAMECARD");
}
void getAName(int i, char *buffer) {
	strcpy(buffer, "UNKNOWN");
	if (i==3) strcpy(buffer, "ROMFS");
	if (i==4) strcpy(buffer, "SAVEDATA");
	if (i==6) strcpy(buffer, "EXTDATA");
	if (i==7) strcpy(buffer, "SHARED_EXTDATA");
	if (i==8) strcpy(buffer, "SYSTEM_SAVEDATA");
	if (i==9) strcpy(buffer, "SDMC");
	if (i==0xA) strcpy(buffer, "SDMC_WRITE_ONLY");
	if (i==0x12345678) strcpy(buffer, "BOSS_EXTDATA");
	if (i==0x12345679) strcpy(buffer, "CARD_SPIFS");
	if (i==0x1234567D) strcpy(buffer, "NAND_RW");
	if (i==0x1234567E) strcpy(buffer, "NAND_RO");
	if (i==0x1234567F) strcpy(buffer, "NAND_RO_WRITE_ACCESS");
}

void bar(mediatypes_enum mediatype, int i, FS_archiveIds archivetype, char *dumpfolder) {
	u32 extdata_archive_lowpathdata[3] = {mediatype, i, 0};
	extdata_archive = (FS_archive){archivetype, (FS_path){PATH_BINARY, 0xC, (u8*)extdata_archive_lowpathdata}};
	
	Result ret = FSUSER_OpenArchive(NULL, &extdata_archive);
	if(ret!=0)
	{
		return;
	}
	
	printf("Archive 0x%08x opened.\n", (unsigned int) archivetype);
	mkdir(dumpfolder, 0777);
	foo("/", i, dumpfolder);
	
	FSUSER_CloseArchive(NULL, &extdata_archive);
}

Result open_extdata()
{
	Result ret=0;
	u8 region=0;
	
	filebuffer = (u8*)malloc(q);
	memset(filebuffer, 0, q);

	ret = initCfgu();
	if(ret!=0)
	{
		printf("initCfgu() failed: 0x%08x\n", (unsigned int)ret);
		gfxFlushBuffers();
		gfxSwapBuffers();
		free(filebuffer);
		return ret;
	}

	ret = CFGU_SecureInfoGetRegion(&region);
	if(ret!=0)
	{
		printf("CFGU_SecureInfoGetRegion() failed: 0x%08x\n", (unsigned int)ret);
		gfxFlushBuffers();
		gfxSwapBuffers();
		free(filebuffer);
		return ret;
	}

	exitCfgu();

	int i;
	for (i=0x00000000; i<0x00002000; ++i) {
		bar(mediatype_SDMC, i, ARCH_EXTDATA, "user_extdata");
	}
	for (i=0xE0000000; i<0xE0000100; ++i) {
		bar(mediatype_NAND, i, ARCH_SHARED_EXTDATA, "shared_extdata");
	}
	for (i=0xF0000000; i<0xF0000100; ++i) {
		bar(mediatype_NAND, i, ARCH_SHARED_EXTDATA, "shared_extdata");
	}
	
	printf("Success!\n");
	gfxFlushBuffers();
	gfxSwapBuffers();
	//svcSleepThread(5000000000LL);

	free(filebuffer);
	
	return 0;
}

void close_extdata()
{

}

Result archive_getfilesize(Archive archive, char *path, u32 *outsize)
{
	Result ret=0;
	struct stat filestats;
	u64 tmp64=0;
	Handle filehandle=0;

	char filepath[256];

	if(archive==SDArchive)
	{
		memset(filepath, 0, 256);
		strncpy(filepath, path, 255);

		if(stat(filepath, &filestats)==-1)return errno;

		*outsize = filestats.st_size;

		return 0;
	}

	ret = FSUSER_OpenFile(NULL, &filehandle, extdata_archive, FS_makePath(PATH_CHAR, path), 1, 0);
	if(ret!=0)return ret;

	ret = FSFILE_GetSize(filehandle, &tmp64);
	if(ret==0)*outsize = (u32)tmp64;

	FSFILE_Close(filehandle);

	return ret;
}

Result archive_readfile(Archive archive, char *path, u8 *buffer, u32 size)
{
	Result ret=0;
	Handle filehandle=0;
	u32 tmpval=0;
	FILE *f;

	char filepath[256];

	if(archive==SDArchive)
	{
		memset(filepath, 0, 256);
		strncpy(filepath, path, 255);

		f = fopen(filepath, "r");
		if(f==NULL)return errno;

		tmpval = fread(buffer, 1, size, f);

		fclose(f);

		if(tmpval!=size)return -2;

		return 0;
	}

	ret = FSUSER_OpenFile(NULL, &filehandle, extdata_archive, FS_makePath(PATH_CHAR, path), FS_OPEN_READ, 0);
	if(ret!=0)return ret;

	ret = FSFILE_Read(filehandle, &tmpval, 0, buffer, size);

	FSFILE_Close(filehandle);

	if(ret==0 && tmpval!=size)ret=-2;

	return ret;
}

Result archive_writefile(Archive archive, char *path, u8 *buffer, u32 size)
{
	Result ret=0;
	Handle filehandle=0;
	u32 tmpval=0;
	FILE *f;

	char filepath[256];

	if(archive==SDArchive)
	{
		memset(filepath, 0, 256);
		strncpy(filepath, path, 255);

		f = fopen(filepath, "w+");
		if(f==NULL)return errno;

		tmpval = fwrite(buffer, 1, size, f);

		fclose(f);

		if(tmpval!=size)return -2;

		return 0;
	}

	ret = FSUSER_OpenFile(NULL, &filehandle, extdata_archive, FS_makePath(PATH_CHAR, path), FS_OPEN_WRITE, 0);
	if(ret!=0)return ret;

	ret = FSFILE_Write(filehandle, &tmpval, 0, buffer, size, FS_WRITE_FLUSH);

	FSFILE_Close(filehandle);

	if(ret==0 && tmpval!=size)ret=-2;

	return ret;
}

Result archive_copyfile(Archive inarchive, Archive outarchive, char *inpath, char *outpath, u8* buffer, u32 size, u32 maxbufsize, char *display_filepath)
{
	Result ret=0;
	u32 filesize=0;

	ret = archive_getfilesize(inarchive, inpath, &filesize);
	if (ret > 0) {
		printf("archive_getfilesize() ret=0x%08x, size=0x%08x\n", (unsigned int)ret, (unsigned int)filesize);
	}
	gfxFlushBuffers();
	gfxSwapBuffers();
	if(ret!=0)return ret;

	if(size==0 || size>filesize)
	{
		size = filesize;
	}

	if(size>maxbufsize)
	{
		printf("~~~~~Size is too large.~~~~~\n");
		gfxFlushBuffers();
		gfxSwapBuffers();
		ret = -1;
		return ret;
	}

	//printf("Reading %s...\n", display_filepath);
	gfxFlushBuffers();
	gfxSwapBuffers();

	ret = archive_readfile(inarchive, inpath, buffer, size);
	if(ret!=0)
	{
		printf("-----Failed to read %s (%d): 0x%08x-----\n", display_filepath, (unsigned int) size, (unsigned int)ret);
		gfxFlushBuffers();
		gfxSwapBuffers();
		return ret;
	}

	//printf("Writing %s...\n", display_filepath);
	gfxFlushBuffers();
	gfxSwapBuffers();

	ret = archive_writefile(outarchive, outpath, buffer, size);
	if(ret!=0)
	{
		printf("-----Failed to write %s (%d): 0x%08x-----\n", display_filepath, (unsigned int) size, (unsigned int)ret);
		gfxFlushBuffers();
		gfxSwapBuffers();
		return ret;
	}

	return ret;
}

