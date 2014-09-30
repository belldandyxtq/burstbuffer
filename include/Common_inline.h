/*all inline function*/


#ifndef COMMON_INLINE_H_
#define COMMON_INLINE_H_

inline void set_aiocb(struct aiocb *cb, int fd, off_t offset, void *buffer, size_t nbytes, int opcode, const struct sigevent *event);

#endif
