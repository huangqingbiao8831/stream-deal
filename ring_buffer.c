#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>

#include "ring_buffer.h"
/***************************************************************
  删除线程锁：
            只在收消息侧进行处理，不需要锁
			huangqb@20201226
****************************************************************/

ssize_t      /* Read "n" bytes from a descriptor. */  
readn(int fd, void *vptr, size_t n)  
{  
 size_t nleft;  
 ssize_t nread;  
 char *ptr;  
  
 ptr = vptr;  
 nleft = n;  
 while (nleft > 0) {  
  if ( (nread = read(fd, ptr, nleft)) < 0) {  
   if (errno == EINTR)  
    nread = 0;  /* and call read() again */  
   else  
    return(-1);  
  } else if (nread == 0)  
   break;    /* EOF */  
  
  nleft -= nread;  
  ptr   += nread;  
 }  
 return(n - nleft);  /* return >= 0 */  
}  

/********************************************
write length
*********************************************/
ssize_t      /* Write "n" bytes to a descriptor. */  
writen(int fd, const void *vptr, size_t n)  
{  
 size_t  nleft;  
 ssize_t  nwritten;  
 const char *ptr;  
  
 ptr = vptr;  
 nleft = n;  
 while (nleft > 0) {  
  if ( (nwritten = write(fd, ptr, nleft)) <= 0) {  
   if (nwritten < 0 && errno == EINTR)  
    nwritten = 0;  /* and call write() again */  
   else  
    return(-1);   /* error */  
  }  
  
  nleft -= nwritten;  
  ptr   += nwritten;  
 }  
 return(n);  
}  


struct ring_buffer* ring_buffer_init(void *buffer, uint32_t size, pthread_mutex_t *f_lock)
{
    assert(buffer);
    struct ring_buffer *ring_buf = NULL;
    if (!is_power_of_2(size))
    {
    fprintf(stderr,"size must be power of 2.\n");
        return ring_buf;
    }
    ring_buf = (struct ring_buffer *)malloc(sizeof(struct ring_buffer));
    if (!ring_buf)
    {
        fprintf(stderr,"Failed to malloc memory,errno:%u,reason:%s",
            errno, strerror(errno));
        return ring_buf;
    }
    memset(ring_buf, 0, sizeof(struct ring_buffer));
    ring_buf->buffer = buffer;
    ring_buf->size = size;
    ring_buf->in = 0;
    ring_buf->out = 0;
    //ring_buf->f_lock = f_lock; //in one thread donnot need...
    return ring_buf;
}

void ring_buffer_free(struct ring_buffer *ring_buf)
{
    if (ring_buf)
    {
    if (ring_buf->buffer)
    {
        free(ring_buf->buffer);
        ring_buf->buffer = NULL;
    }
    free(ring_buf);
    ring_buf = NULL;
    }
}

uint32_t __ring_buffer_len(const struct ring_buffer *ring_buf)
{
    return (ring_buf->in - ring_buf->out);
}


uint32_t __ring_buffer_get(struct ring_buffer *ring_buf, void * buffer, uint32_t size)
{
    assert(ring_buf || buffer);
    uint32_t len = 0;
    size  = min(size, ring_buf->in - ring_buf->out);        
    /* first get the data from fifo->out until the end of the buffer */
    len = min(size, ring_buf->size - (ring_buf->out & (ring_buf->size - 1)));
    memcpy(buffer, ring_buf->buffer + (ring_buf->out & (ring_buf->size - 1)), len);
    /* then get the rest (if any) from the beginning of the buffer */
    memcpy(buffer + len, ring_buf->buffer, size - len);
    ring_buf->out += size;
    return size;
}



uint32_t __ring_buffer_put(struct ring_buffer *ring_buf, void *buffer, uint32_t size)
{
    assert(ring_buf || buffer);
    uint32_t len = 0;

   /*
    * ring_buf->size - in + out : 
    * indict  store max  space
    * */

    size = min(size, ring_buf->size - ring_buf->in + ring_buf->out);

    /* first put the data starting from fifo->in to buffer end */
    len  = min(size, ring_buf->size - (ring_buf->in & (ring_buf->size - 1)));

    /*
	 *            len: max is  from fifo->in to buffer end
 	*
 	* */

    memcpy(ring_buf->buffer + (ring_buf->in & (ring_buf->size - 1)), buffer, len);

    /* then put the rest (if any) at the beginning of the buffer */
    memcpy(ring_buf->buffer, buffer + len, size - len);
    ring_buf->in += size;
    return size;
}

uint32_t ring_buffer_len(const struct ring_buffer *ring_buf)
{
    uint32_t len = 0;
    //pthread_mutex_lock(ring_buf->f_lock);
    len = __ring_buffer_len(ring_buf);
    //pthread_mutex_unlock(ring_buf->f_lock);
    return len;
}

uint32_t ring_buffer_get(struct ring_buffer *ring_buf, void *buffer, uint32_t size)
{
    uint32_t ret;
    //pthread_mutex_lock(ring_buf->f_lock);
    ret = __ring_buffer_get(ring_buf, buffer, size);
    if (ring_buf->in == ring_buf->out)
    ring_buf->in = ring_buf->out = 0;
    //pthread_mutex_unlock(ring_buf->f_lock);
    return ret;
}

uint32_t ring_buffer_put(struct ring_buffer *ring_buf, void *buffer, uint32_t size)
{
    uint32_t ret;
    //pthread_mutex_lock(ring_buf->f_lock);
    ret = __ring_buffer_put(ring_buf, buffer, size);
    //pthread_mutex_unlock(ring_buf->f_lock);
    return ret;
}
