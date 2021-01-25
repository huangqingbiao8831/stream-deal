#include <stdio.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>
#include "ring_buffer.h"
 
// the max connection number of the server
#define MAX_CONNECTION_NUMBER 2
 
/* * Create a server endpoint of a connection. * Returns fd if all OK, <0 on error. */
static int unix_socket_listen(const char *servername)
{ 
  int fd;
  struct sockaddr_un un; 
  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
  {
  	 return(-1); 
  }
  int len, rval; 
  unlink(servername);               /* in case it already exists */ 
  memset(&un, 0, sizeof(un)); 
  un.sun_family = AF_UNIX; 
  strcpy(un.sun_path, servername); 
  len = offsetof(struct sockaddr_un, sun_path) + strlen(servername); 
  /* bind the name to the descriptor */ 
  if (bind(fd, (struct sockaddr *)&un, len) < 0)
  { 
    rval = -2; 
  } 
  else
  {
	  if (listen(fd, MAX_CONNECTION_NUMBER) < 0)    
	  { 
		rval =  -3; 
	  }
	  else
	  {
	    return fd;
	  }
  }
  int err;
  err = errno;
  close(fd); 
  errno = err;
  return rval;	
}
 
static int unix_socket_accept(int listenfd, uid_t *uidptr)
{ 
   int clifd, len, rval; 
   time_t staletime; 
   struct sockaddr_un un;
   struct stat statbuf; 
   len = sizeof(un); 
   if ((clifd = accept(listenfd, (struct sockaddr *)&un, &len)) < 0) 
   {
      return(-1);     
   }
 /* obtain the client's uid from its calling address */ 
   len -= offsetof(struct sockaddr_un, sun_path);  /* len of pathname */
   un.sun_path[len] = 0; /* null terminate */ 
   if (stat(un.sun_path, &statbuf) < 0) 
   {
      rval = -2;
   } 
   else
   {
	   if (S_ISSOCK(statbuf.st_mode) ) 
	   { 
		  if (uidptr != NULL) *uidptr = statbuf.st_uid;    /* return uid of caller */ 
	      unlink(un.sun_path);       /* we're done with pathname now */ 
		  return clifd;		 
	   } 
	   else
	   {
	      rval = -3;     /* not a socket */ 
	   }
    }
   int err;
   err = errno; 
   close(clifd); 
   errno = err;
   return(rval);
 }
 
 static void unix_socket_close(int fd)
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

static void printHexStr(char *pMsg,int msgLen)
{
   int i =0;
   int j = 0;
  printf("recv msg:\n");
  for (i = 0;i < msgLen;i++)
  {
     if (i%16 == 0) printf("\n");
     printf(" %02x ",pMsg[i]);
  }
  printf("\n--------------------------------------------\n");
   
}

static int ServiceHandler(char *pMsg,int msgLen)
{
  printf("handler msg \n");
  printHexStr(pMsg,msgLen);
  return 0;
}

static void stRecvMsgSpeed(int msgLen)
{
  static long long count = 0;
  static long long totalMsgLen = 0;
  static long long speed = 0;
  static long long durTime = 0;
  static struct timeval start_tv;
  static struct timeval end_tv;

  gettimeofday(&end_tv, 0);
  count++;
  totalMsgLen += msgLen;
  //statis window 
  if (count > 10000)
  {
     durTime = (end_tv.tv_sec - start_tv.tv_sec) * 1000000LL + (end_tv.tv_usec - start_tv.tv_usec);
     speed = totalMsgLen *1000000LL*8/durTime/1024/1024;
     printf("speed:%d Mbps\n",speed);    
     gettimeofday(&start_tv, 0); 
     count = 0;
     totalMsgLen = 0; 
  } 
}

void *unix_server(void *arg)
{ 
	int listenfd,connfd;
	char *pReadBuf = NULL;
	char *dealBuf = NULL;
	char *leftBuf = NULL;
	//struct ring_buffer *ring_buf = NULL; //环形队列控制块
	int fd_size = 0;
	int dw;
	int size;
	struct timeval timeout;
	struct socket_msg_header *pMsg = NULL;
	uint32_t readLen = 0;
	fd_set fdset;
	uint32_t msgLen = 0;
	uint32_t header;
        uint32_t counter = 0;
        

        printf("msg header:%d \n",sizeof(struct socket_msg_header));

          
	while(1)
	{
		listenfd = unix_socket_listen("foo.sock");
		if(listenfd<0)
		{
			printf("Error[%d] when listening...\n",errno);
			continue;
		}
		printf("Finished listening...\n",errno);
		uid_t uid;
		connfd = unix_socket_accept(listenfd, &uid);
		unix_socket_close(listenfd);  
		if(connfd<0)
		{
			printf("Error[%d] when accepting...\n",errno);
			continue;
		}  
		printf("Begin to recv/send...\n");  
		int i,n,size, rs;
		int leftLen = 0;
		int willDealLen = 0;
		//char rvbuf[2048];
		pReadBuf = (char*)malloc(8*1024);//malloc 8K buffer...,recv缓存... 
		dealBuf = (char *)malloc(2*1024*1024); //malloc 1M buffer
		leftBuf = (char *)malloc(8*1024); //deal msg buffer...
		char *ptempMsg = NULL;

		fd_size = connfd+1;
		while(1)
		{
			//recv data and save to ringBuffer...,if error ,break while
			FD_ZERO(&fdset);
			FD_SET (connfd, &fdset);
			//FD_SET (udp_ack_fd, &fdset);
			timeout.tv_sec=1;  // 10 seconds timerout
			timeout.tv_usec=0;
			dw = select(fd_size,&fdset,NULL,NULL,&timeout);
			if(dw < 0)
			{
				//error....
			   printf("select < 0");
			   break;
			}
			else if (0==dw)
			{
                           //timeout...
			   printf("dw==0 ,timeout....");
			}
			else
			{
			  if (FD_ISSET(connfd,&fdset))
			  {
			     size = recv(connfd,pReadBuf,8*1024,0);
			     //printf("recv msg len:%d ,leftLen =%d\n",size,leftLen);
			     stRecvMsgSpeed(size);
			     memset(dealBuf,0,2*1024*1024); //
			     if (leftLen > 0)//if leftbuf is not NULL
			     {
				 memcpy(dealBuf,leftBuf,leftLen);
				 memset(leftBuf,0,8*1024);
                                 //printf("copy from left byte:%d,%02x,%02x,%02x,%02x \n",leftLen,dealBuf[0],dealBuf[1],dealBuf[2],dealBuf[3]);
                                   
			     }
			     memcpy(dealBuf+leftLen,pReadBuf,size);
			     willDealLen = size + leftLen;
                             leftLen = 0;//after copy ,must reset leftLen
			     readLen = 0;
                             counter = 0;
                             //printf("Will Enter read Msg: willDealLen:%d\n",willDealLen);
			     while( willDealLen > (MSG_HEADER_LEN))
			     {
			         pMsg = (struct socket_msg_header*)&dealBuf[readLen];
				 msgLen = ntohl(pMsg->msgLen);
				 header = ntohl(pMsg->header);
				 //printf("current msgLen=%d,header=0x%08x,total len=%d \n",msgLen,header,willDealLen);
				 if (header != 0x1A2B3C4D)
				 {
					//printf("msg header error:header=0x%08x ,willDealLen=%d,leftLen=%d,msgLen=%d\n",header,willDealLen,leftLen,msgLen);
                                        //printHexStr(dealBuf,willDealLen);
					exit(1); 
				 }
				 if ( willDealLen < ((MSG_HEADER_LEN) + msgLen)) //not enough one packag 
				 { 
					//printf("left msg can not package...willDealLen=%d,msgLen=%d\n",willDealLen,msgLen);
                                        //printHexStr(dealBuf,leftLen);
					break;
				 }
				 else
				 {
                                        counter++;
					willDealLen -= (msgLen + MSG_HEADER_LEN);
                                        //ServiceHandler(pMsg->pMsgBody,msgLen);
                                        //ServiceHandler((char*)pMsg,msgLen);
					readLen += (msgLen + MSG_HEADER_LEN);
					//printf("have left %d,have read: %d\n",willDealLen,readLen);
			         }
			      }
                              //printf("read counter :%d \n",counter);
			      if (willDealLen > 0 )
			       {
                                  memcpy(leftBuf,&dealBuf[readLen],willDealLen);
			          leftLen = willDealLen;
                                  //printf("copy to left buffer:%d \n",leftLen);
                                  //printHexStr(leftBuf,leftLen);
			       }
			  }
                    }
			//sleep(1);
		}
		unix_socket_close(connfd);
		free(pReadBuf);
		//free(pBuffer);
		pReadBuf = NULL;
		//pBuffer = NULL;
		//ring_buffer_free(ring_buf);
	}
     printf("Server exited.\n");   
     exit(1); 
}
