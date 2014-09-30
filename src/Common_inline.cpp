

#include <string.h>
#include <aio.h>

#include "include/Common_inline.h"
#include "include/IO_const.h"

inline void set_aiocb(struct aiocb *cb, int fd, off_t offset, void *buffer, size_t nbytes, int opcode, const struct sigevent *event)
{
	memset(cb, 0, sizeof(struct aiocb));
	cb->aio_filedfs=fd;
	cb->aio_offset=offset;
	cb->aio_buf-buffer;
	cb->aio_nbytes=nbytes;
	cb->aio_lio_opcode=opcode;
	memcpy(&cb->aio_sigevent, event, sizeof(struct sigevent));
}
