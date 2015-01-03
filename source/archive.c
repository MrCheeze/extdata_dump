#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <3ds.h>
#include <stdlib.h>

#include "archive.h"

FS_archive extdata_archive;
u8 *filebuffer;
size_t bufsize = 0x1000000;

void unicodeToChar(char* dst, u16* src) {
	if(!src || !dst)return;
	while(*src)*(dst++)=(*(src++))&0xFF;
	*dst=0x00;
}

void dumpFolder(char *path, u32 lowpath_id, char *dumpfolder) {
	Handle extdata_dir;
	Result ret = FSUSER_OpenDirectory(NULL, &extdata_dir, extdata_archive, FS_makePath(PATH_CHAR, path));
	if (ret!=0) {
		printf("could not open dir\n");
		gfxFlushBuffers();
		gfxSwapBuffers();
		return;
	}
	char dirname[0x120];
	sprintf(dirname, "%s/%08x%s", dumpfolder, (unsigned int) lowpath_id, path);
	mkdir(dirname, 0777);
	
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
			dumpFolder(newpath, lowpath_id, dumpfolder);
		} else {
			char file_inpath[0x120];
			char file_outpath[0x120];
			char file_display_path[0x120];

			sprintf(file_inpath, "%s%s", path, fileName);
			sprintf(file_outpath, "%s/%08x%s%s", dumpfolder, (unsigned int) lowpath_id, path, fileName);
			sprintf(file_display_path, "%08x%s%s", (unsigned int) lowpath_id, path, fileName);
			archive_copyfile(Extdata_Archive, SDArchive, file_inpath, file_outpath, filebuffer, 0, bufsize, file_display_path);
		}
	}
	printf("total files in 0x%08x%s: %d\n", (unsigned int) lowpath_id, path, (unsigned int) cont);
	gfxFlushBuffers();
	gfxSwapBuffers();
	FSDIR_Close(extdata_dir);
}

void getMName(int i, char *buffer) {
	switch (i) {
		case 0:
			strcpy(buffer, "NAND");
			break;
		case 1:	
			strcpy(buffer, "SDMC");
			break;
		case 2:	
			strcpy(buffer, "GAMECARD");
			break;
		default:
			strcpy(buffer, "UNKNOWN");
			break;
	}
}
void getAName(int i, char *buffer) {
	switch (i) {
		case 3:
			strcpy(buffer, "ROMFS");
			break;
		case 4:
			strcpy(buffer, "SAVEDATA");
			break;
		case 6:
			strcpy(buffer, "EXTDATA");
			break;
		case 7:
			strcpy(buffer, "SHARED_EXTDATA");
			break;
		case 8:
			strcpy(buffer, "SYSTEM_SAVEDATA");
			break;
		case 9:
			strcpy(buffer, "SDMC");
			break;
		case 0xA:
			strcpy(buffer, "SDMC_WRITE_ONLY");
			break;
		case 0x12345678:
			strcpy(buffer, "BOSS_EXTDATA");
			break;
		case 0x12345679:
			strcpy(buffer, "CARD_SPIFS");
			break;
		case 0x1234567D:
			strcpy(buffer, "NAND_RW");
			break;
		case 0x1234567E:
			strcpy(buffer, "NAND_RO");
			break;
		case 0x1234567F:
			strcpy(buffer, "NAND_RO_WRITE_ACCESS");
			break;
		default:
			strcpy(buffer, "UNKNOWN");
			break;
	}
}

void dumpArchive(mediatypes_enum mediatype, int i, FS_archiveIds archivetype, char *dirpath) {
	u32 extdata_archive_lowpathdata[3] = {mediatype, i, 0};
	extdata_archive = (FS_archive){archivetype, (FS_path){PATH_BINARY, 0xC, (u8*)extdata_archive_lowpathdata}};
	
	Result ret = FSUSER_OpenArchive(NULL, &extdata_archive);
	if(ret!=0)
	{
		return;
	}
	
	printf("Archive 0x%08x opened.\n", (unsigned int) archivetype);
	gfxFlushBuffers();
	gfxSwapBuffers();
	mkdir(dirpath, 0777);
	dumpFolder("/", i, dirpath);
	
	FSUSER_CloseArchive(NULL, &extdata_archive);
}

Result backupAllExtdata()
{
	Result ret=0;
	u8 region=0;
	
	filebuffer = (u8*)malloc(bufsize);
	memset(filebuffer, 0, bufsize);

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
		dumpArchive(mediatype_SDMC, i, ARCH_EXTDATA, "user_extdata");
	}
	for (i=0xE0000000; i<0xE0000100; ++i) {
		dumpArchive(mediatype_NAND, i, ARCH_SHARED_EXTDATA, "shared_extdata");
	}
	for (i=0xF0000000; i<0xF0000100; ++i) {
		dumpArchive(mediatype_NAND, i, ARCH_SHARED_EXTDATA, "shared_extdata");
	}
	
	printf("Success!\n");
	gfxFlushBuffers();
	gfxSwapBuffers();
	//svcSleepThread(5000000000LL);

	free(filebuffer);
	
	return 0;
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
		gfxFlushBuffers();
		gfxSwapBuffers();
	}
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

	/*printf("Reading %s...\n", display_filepath);
	gfxFlushBuffers();
	gfxSwapBuffers();*/

	ret = archive_readfile(inarchive, inpath, buffer, size);
	if(ret!=0)
	{
		printf("-----Failed to read %s (%d): 0x%08x-----\n", display_filepath, (unsigned int) size, (unsigned int)ret);
		gfxFlushBuffers();
		gfxSwapBuffers();
		return ret;
	}

	/*printf("Writing %s...\n", display_filepath);
	gfxFlushBuffers();
	gfxSwapBuffers();*/

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

