#ifndef __RING_BUFFER_H__
#define __RING_BUFFER_H__
#include <stdint.h>
#include <stdio.h>
#include <pthread.h>

#define is_power_of_2(x) ((x) != 0 && (((x) & ((x) - 1)) == 0))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define MSG_HEADER_LEN      40

struct ring_buffer
{
    void         *buffer;   
    uint32_t     size;   
    uint32_t     in;   
    uint32_t       out;     
    //pthread_mutex_t *f_lock;  
};

//define msg header
struct socket_msg_header
{
   uint32_t header;
   uint32_t msgLen;
   char routingKey[32];
   char pMsgBody[1];
}; 

struct ring_buffer* ring_buffer_init(void *buffer, uint32_t size, pthread_mutex_t *f_lock);
ssize_t readn(int fd, void *vptr, size_t n);
ssize_t writen(int fd, const void *vptr, size_t n);

#endif
