#include <stdio.h>
#include <stddef.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include "ring_buffer.h"
#include <time.h> 
/* Create a client endpoint and connect to a server.   Returns fd if all OK, <0 on error. */
int unix_socket_conn(const char *servername)
{ 
  int fd; 
  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)    /* create a UNIX domain stream socket */ 
  {
    return(-1);
  }
  int len, rval;
   struct sockaddr_un un;          
  memset(&un, 0, sizeof(un));            /* fill socket address structure with our address */
  un.sun_family = AF_UNIX; 
  sprintf(un.sun_path, "scktmp%05d", getpid()); 
  len = offsetof(struct sockaddr_un, sun_path) + strlen(un.sun_path);
  unlink(un.sun_path);               /* in case it already exists */ 
  if (bind(fd, (struct sockaddr *)&un, len) < 0)
  { 
  	 rval=  -2; 
  } 
  else
  {
	/* fill socket address structure with server's address */
	  memset(&un, 0, sizeof(un)); 
	  un.sun_family = AF_UNIX; 
	  strcpy(un.sun_path, servername); 
	  len = offsetof(struct sockaddr_un, sun_path) + strlen(servername); 
	  if (connect(fd, (struct sockaddr *)&un, len) < 0) 
	  {
		  rval= -4; 
	  } 
	  else
	  {
	     return (fd);
	  }
  }
  int err;
  err = errno;
  close(fd); 
  errno = err;
  return rval;	  
}
 
 void unix_socket_close(int fd)
 {
    close(fd);     
 }

static int EslThreadDelay(const long lTimeSec, const long lTimeUSec)
{
	struct timeval timeOut;
	timeOut.tv_sec = lTimeSec;
	timeOut.tv_usec = lTimeUSec;
	if (0 != select(0, NULL, NULL, NULL, &timeOut))
	{
		return 1;
	}
	return 0;
}
 
 
void *unix_client(void *arg)
{ 
  //srand((int)time(0));
  int connfd; 
  int i,n,size;
  char *letter="abcdefjhijklmnobqrstuvwxyz";
  struct socket_msg_header *pHeader = NULL;
  unsigned int counter = 0; 

  while(1)
  {
		connfd = unix_socket_conn("foo.sock");
		if(connfd<0)
		{
			printf("Error[%d] when connecting...",errno);
			continue;
		}
		printf("SEND:Begin to recv/send...\n");  
		
		char sendbuf[4096];
		struct socket_msg_header *pmsgHeader = (struct socket_msg_header*)sendbuf;
		
		while(1)
		{
			//memset(sendbuf,0,4096);
			srand((unsigned)time(NULL));
			n = rand()%3000;
			i = rand()%26;
			memset(sendbuf,letter[i],n);
                       
			//construct header...
                        pmsgHeader->header = htonl(0x1A2B3C4D); //
                        if( n <= MSG_HEADER_LEN ) continue;
		        pmsgHeader->msgLen = htonl(n - MSG_HEADER_LEN); //msgLen
			//printf("SEND:------------>send msgLen=%d,msgheader=0x%08x\n",pmsgHeader->msgLen,pmsgHeader->header);
            
			size = writen(connfd,(char *)sendbuf,n);
			if (size != n)
			{
			    printf("SEND:send msg header error \n");
			    break;
			}
                        counter++;
                        if (counter > 5)
                        {
			  //EslThreadDelay(0,100);
                          counter = 0;
                        }
			//sleep(1);
			//EslThreadDelay(0,100);
		}
		unix_socket_close(connfd);
  }
   printf("Client exited.\n");    
 
}

