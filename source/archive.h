typedef enum {
	Extdata_Archive,
	SDArchive
} Archive;

Result backupAllExtdata(u8 *filebuffer, size_t bufsize);
Result restoreFromSd(u8 *filebuffer, size_t bufsize);
Result backupByConfig(u8 *filebuffer, size_t bufsize);
Result archive_getfilesize(Archive archive, char *path, u32 *outsize);
Result archive_readfile(Archive archive, char *path, u8 *buffer, u32 size);
Result archive_writefile(Archive archive, char *path, u8 *buffer, u32 size);
Result archive_copyfile(Archive inarchive, Archive outarchive, char *inpath, char *outpath, u8* buffer, u32 size, u32 maxbufsize, char *display_filepath);

