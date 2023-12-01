#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <errno.h>
#include <3ds.h>
#include <stdlib.h>

#include "archive.h"
#include "enumerate_extdata.h"

/* this code sucks, fyi */

char *user_extdata_dumpfolder = "dumps";
char *shared_extdata_dumpfolder = "dumps";

FS_Archive extdata_archive;

static int maybe_mkdir(const char* path)
{
    struct stat st;
    errno = 0;

    /* Try to make the directory */
    if (mkdir(path, 0777) == 0)
        return 0;

    /* If it fails for any reason but EEXIST, fail */
    if (errno != EEXIST)
        return -1;

    /* Check if the existing path is a directory */
    if (stat(path, &st) != 0)
        return -1;

    /* If not, fail with ENOTDIR */
    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return -1;
    }

    errno = 0;
    return 0;
}

static int mkdir_p(const char *path)
{
    /* Adapted from http://stackoverflow.com/a/2336245/119527 */
    char *p;
    int result = -1;

    errno = 0;

    printf("making dir: %s\n", path);

    /* Copy string so it's mutable */
    char *_path = strdup(path);
    if (_path == NULL)
        goto out;

    /* Iterate the string */
    for (p = _path + 1; *p; p++) {
        if (*p == '/') {
            /* Temporarily truncate */
            *p = '\0';

            if (maybe_mkdir(_path) != 0)
                goto out;

            *p = '/';
        }
    }

    if (maybe_mkdir(_path) != 0)
        goto out;

    result = 0;

out:
    free(_path);
    return result;
}

void unicodeToChar(char* dst, u16* src) {
	if(!src || !dst)return;
	while(*src)*(dst++)=(*(src++))&0xFF;
	*dst=0x00;
}

void dumpFolder(char *path, u32 lowpath_id, char *dumpfolder, u8 *filebuffer, size_t bufsize) {
	Handle extdata_dir;
	Result ret = FSUSER_OpenDirectory(&extdata_dir, extdata_archive, fsMakePath(PATH_ASCII, path));
	if (ret!=0) {
		printf("could not open dir. Error 0x%lx\n", ret);
		gfxFlushBuffers();
		gfxSwapBuffers();
		return;
	}
	char dirname[0x120];
	sprintf(dirname, "%s/%08x%s", dumpfolder, (unsigned int) lowpath_id, path);
	mkdir_p(dirname);
	
	FS_DirectoryEntry dirStruct;
	char fileName[0x106] = "";
	int cont = 0;
	while(1) {
		u32 dataRead = 0;
		FSDIR_Read(extdata_dir, &dataRead, 1, &dirStruct);
		if(dataRead == 0) break;
		unicodeToChar(fileName, dirStruct.name);
		printf("name: %s%s%s\n", path, fileName, (dirStruct.attributes & FS_ATTRIBUTE_DIRECTORY) ? " (DIRECTORY)" : "");
		gfxFlushBuffers();
		gfxSwapBuffers();
		cont++;
		if (dirStruct.attributes & FS_ATTRIBUTE_DIRECTORY) {
			char newpath[0x120];
			sprintf(newpath, "%s%s/", path, fileName);
			printf("%s\n", newpath);
			dumpFolder(newpath, lowpath_id, dumpfolder, filebuffer, bufsize);
		} else {
			char file_inpath[0x120];
			char file_outpath[0x120];
			char file_display_path[0x120];

			sprintf(file_inpath, "%s%s", path, fileName);
			printf("%s\n", file_inpath);
			sprintf(file_outpath, "%s/%08x%s%s", dumpfolder, (unsigned int) lowpath_id, path, fileName);
			printf("%s\n", file_outpath);
			sprintf(file_display_path, "%08x%s%s", (unsigned int) lowpath_id, path, fileName);
			printf("%s\n", file_display_path);
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

void dumpArchive(FS_MediaType mediatype, int i, FS_ArchiveID archivetype, char *dirpath, u8 *filebuffer, size_t bufsize) {
	u32 extdata_archive_lowpathdata[3] = {mediatype, i, 0};

    Result ret = FSUSER_OpenArchive(&extdata_archive, archivetype, (FS_Path) { PATH_BINARY, sizeof(extdata_archive_lowpathdata), extdata_archive_lowpathdata });
	if(ret!=0)
	{
		return;
	}
	
	printf("Archive 0x%08x opened.\n", (unsigned int) i);
	gfxFlushBuffers();
	gfxSwapBuffers();

	mkdir_p(dirpath);

	dumpFolder("/", i, dirpath, filebuffer, bufsize);
	
	FSUSER_CloseArchive(extdata_archive);
}

Result backupAllExtdata(u8 *filebuffer, size_t bufsize)
{
	Result ret=0;
	u8 region=0;
	
	memset(filebuffer, 0, bufsize);

	ret = cfguInit();
	if(ret!=0)
	{
		printf("initCfgu() failed: 0x%08x\n", (unsigned int)ret);
		gfxFlushBuffers();
		gfxSwapBuffers();
		return ret;
	}

	ret = CFGU_SecureInfoGetRegion(&region);
	if(ret!=0)
	{
		printf("CFGU_SecureInfoGetRegion() failed: 0x%08x\n", (unsigned int)ret);
		gfxFlushBuffers();
		gfxSwapBuffers();
		return ret;
	}
	cfguExit();

	
	u32* extdataList = (u32*)malloc(0x10000);
	if (extdataList==NULL)
	{
		printf("malloc failed\n");
		gfxFlushBuffers();
		gfxSwapBuffers();
		return -1;
	}
	int i;
	
	u32 extdataCount = 0;
	ret = EnumerateExtSaveData((u8*) extdataList, 0x10000, &extdataCount, false);
	if(ret!=0)
	{
		printf("EnumerateExtSaveData() failed: 0x%08x\n", (unsigned int)ret);
		gfxFlushBuffers();
		gfxSwapBuffers();
		return ret;
	}
	
	for (i=0; i<extdataCount; ++i) {
		dumpArchive(MEDIATYPE_SD, extdataList[i], ARCHIVE_EXTDATA, user_extdata_dumpfolder, filebuffer, bufsize);
	}
	
	extdataCount = 0;
	ret = EnumerateExtSaveData((u8*) extdataList, 0x10000, &extdataCount, true);
	if(ret!=0)
	{
		printf("EnumerateExtSaveData() failed: 0x%08x\n", (unsigned int)ret);
		gfxFlushBuffers();
		gfxSwapBuffers();
		return ret;
	}
	
	for (i=0; i<extdataCount; ++i) {
		dumpArchive(MEDIATYPE_NAND, extdataList[i], ARCHIVE_SHARED_EXTDATA, shared_extdata_dumpfolder, filebuffer, bufsize);
	}
	free(extdataList);
	
	printf("Success!\n");
	gfxFlushBuffers();
	gfxSwapBuffers();
	
	return 0;
}

Result restoreFromSd(u8 *filebuffer, size_t bufsize) {
	memset(filebuffer, 0, bufsize);
	
	FILE *configfile = fopen("config.txt", "r");
	if (configfile==NULL) return errno;

	char sd_source[1000], destination_path[1000];
	u32 destination_archive;
	FS_MediaType mtype;
	FS_ArchiveID atype;
	int buf2size = 0x10000;
	u8 *buf2 = malloc(buf2size);
	if (buf2==NULL)
	{
		printf("malloc failed\n");
		gfxFlushBuffers();
		gfxSwapBuffers();
		return -1;
	}
	memset(buf2, 0, buf2size);
	
	while (fgets((char*) buf2, buf2size, configfile) != NULL) {
		if (sscanf((const char*) buf2, "RESTORE \"%999[^\"]\" \"%x:%999[^\"]\"", sd_source, (unsigned int*) &destination_archive, destination_path) == 3) {
			if (destination_archive >= 0x80000000) {
				mtype = MEDIATYPE_NAND;
				atype = ARCHIVE_SHARED_EXTDATA;
			}
			else {
				mtype = MEDIATYPE_SD;
				atype = ARCHIVE_EXTDATA;
			}
			u32 extdata_archive_lowpathdata[3] = {mtype, destination_archive, 0};

    		Result ret = FSUSER_OpenArchive(&extdata_archive, atype, (FS_Path) { PATH_BINARY, sizeof(extdata_archive_lowpathdata), extdata_archive_lowpathdata });
			if(ret!=0)
			{
				printf("could not open archive\n");
				gfxFlushBuffers();
				gfxSwapBuffers();
				continue;
			}
			printf("Restoring to %s\n", destination_path);
			gfxFlushBuffers();
			gfxSwapBuffers();
			ret = archive_copyfile(SDArchive, Extdata_Archive, sd_source, destination_path, filebuffer, 0, bufsize, destination_path);
			if (ret) {
				printf("Copying failed!\n");
			} else {
				printf("Success.\n");
			}
			gfxFlushBuffers();
			gfxSwapBuffers();
			FSUSER_CloseArchive(extdata_archive);
		}
	}
	free(buf2);
	fclose(configfile);
	
	return 0;
}

Result backupByConfig(u8 *filebuffer, size_t bufsize) {
	memset(filebuffer, 0, bufsize);
	
	FILE *configfile = fopen("config.txt", "r");
	if (configfile==NULL) return errno;

	char source_path[1000], sd_destination[1000];
	u32 source_archive;
	FS_MediaType mtype;
	FS_ArchiveID atype;
	int buf2size = 0x10000;
	u8 *buf2 = malloc(buf2size);
	if (buf2==NULL)
	{
		printf("malloc failed\n");
		gfxFlushBuffers();
		gfxSwapBuffers();
		return -1;
	}
	memset(buf2, 0, buf2size);
	
	while (fgets((char*) buf2, buf2size, configfile) != NULL) {
		if (sscanf((const char*) buf2, "DUMP \"%x:%999[^\"]\" \"%999[^\"]\"", (unsigned int*) &source_archive, source_path, sd_destination) == 3) {
			if (source_archive >= 0x80000000) {
				mtype = MEDIATYPE_NAND;
				atype = ARCHIVE_SHARED_EXTDATA;
			}
			else {
				mtype = MEDIATYPE_SD;
				atype = ARCHIVE_EXTDATA;
			}
			u32 extdata_archive_lowpathdata[3] = {mtype, source_archive, 0};

    		Result ret = FSUSER_OpenArchive(&extdata_archive, atype, (FS_Path) { PATH_BINARY, sizeof(extdata_archive_lowpathdata), extdata_archive_lowpathdata });

			if(ret!=0)
			{
				printf("could not open archive\n");
				gfxFlushBuffers();
				gfxSwapBuffers();
				continue;
			}
			printf("Dumping from %s\n", source_path);
			gfxFlushBuffers();
			gfxSwapBuffers();
			ret = archive_copyfile(Extdata_Archive, SDArchive, source_path, sd_destination, filebuffer, 0, bufsize, source_path);
			if (ret) {
				printf("Copying failed!\n");
			} else {
				printf("Success.\n");
			}
			gfxFlushBuffers();
			gfxSwapBuffers();
			FSUSER_CloseArchive(extdata_archive);
		}
	}
	free(buf2);
	fclose(configfile);
	
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

	ret = FSUSER_OpenFile(&filehandle, extdata_archive, fsMakePath(PATH_ASCII, path), 1, 0);
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

	ret = FSUSER_OpenFile(&filehandle, extdata_archive, fsMakePath(PATH_ASCII, path), FS_OPEN_READ, 0);
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

		f = fopen(filepath, "w");
		if(f==NULL) {
			printf("filepath: %s\n", filepath);
			return errno;
		}

		tmpval = fwrite(buffer, 1, size, f);

		fclose(f);

		if(tmpval!=size) return -2;

		return 0;
	}

	FS_Path fspath = fsMakePath(PATH_ASCII, path);
	
	ret = FSUSER_DeleteFile(extdata_archive, fspath);
	
	ret = FSUSER_CreateFile(extdata_archive, fspath, 0, size);
	if(ret!=0) return ret;

	ret = FSUSER_OpenFile(&filehandle, extdata_archive, fspath, FS_OPEN_WRITE, 0);
	if(ret!=0) return ret;

	ret = FSFILE_Write(filehandle, &tmpval, 0, buffer, size, FS_WRITE_FLUSH);

	FSFILE_Close(filehandle);

	if(ret==0 && tmpval!=size) ret=-2;

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
		printf("-----Size is too large.-----\n");
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
		printf("----- %s -------\n", outpath);

		gfxFlushBuffers();
		gfxSwapBuffers();
		while (aptMainLoop())
		{
			gspWaitForVBlank();
			gfxSwapBuffers();
			hidScanInput();

			// Your code goes here
			u32 kDown = hidKeysDown();
			if (kDown & KEY_START)
				break; // break in order to return to hbmenu
		}
		return ret;
	}

	return ret;
}

