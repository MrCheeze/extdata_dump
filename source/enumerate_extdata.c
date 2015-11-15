#include <string.h>
#include <3ds/types.h>
#include <3ds/svc.h>
#include <3ds/srv.h>
#include <3ds/services/fs.h>

Result EnumerateExtSaveData(u32* buffer, u32 bufsize, u32* extdataCount, bool sharedExtdata)
{
	Handle* handle = fsGetSessionHandle();

	u32* cmdbuf = getThreadCommandBuffer();

	cmdbuf[0] = 0x8550102;
	cmdbuf[1] = bufsize;
	cmdbuf[2] = sharedExtdata ? mediatype_NAND : mediatype_SDMC;
	cmdbuf[3] = 4;
	cmdbuf[4] = sharedExtdata ? 1 : 0;
	cmdbuf[5] = (bufsize << 4) | 0xC;
	cmdbuf[6] = (u32) buffer;

	Result ret = 0;
	if((ret = svcSendSyncRequest(*handle)))
		return ret;

	*extdataCount = cmdbuf[2];
	return cmdbuf[1];
}