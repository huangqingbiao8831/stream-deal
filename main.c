#include "ring_buffer.h"
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define BUFFER_SIZE  4096 


extern void *unix_server(void *arg);//consumer
extern void *unix_client(void *arg);//produce

typedef struct student_info
{
    uint64_t stu_id;
    uint32_t age;
    uint32_t score;
}student_info;


void print_student_info(const student_info *stu_info)
{
    assert(stu_info);
    printf("id:%lu\t",stu_info->stu_id);
    printf("age:%u\t",stu_info->age);
    printf("score:%u\n",stu_info->score);
}

student_info * get_student_info(time_t timer)
{
    student_info *stu_info = (student_info *)malloc(sizeof(student_info));
    if (!stu_info)
    {
    fprintf(stderr, "Failed to malloc memory.\n");
    return NULL;
    }
    srand(timer);
    stu_info->stu_id = 10000 + rand() % 9999;
    stu_info->age = rand() % 30;
    stu_info->score = rand() % 101;
    print_student_info(stu_info);
    return stu_info;
}

void * consumer_proc(void *arg)
{
    struct ring_buffer *ring_buf = (struct ring_buffer *)arg;
    student_info stu_info; 
    while(1)
    {
    //usleep(1000000);
    usleep(2000000);
    printf("------------------------------------------\n");
    printf("get a student info from ring buffer.\n");
    ring_buffer_get(ring_buf, (void *)&stu_info, sizeof(student_info));
    printf("ring buffer length: %u\n", ring_buffer_len(ring_buf));
    print_student_info(&stu_info);
    printf("------------------------------------------\n");
    }
    return (void *)ring_buf;
}

void * producer_proc(void *arg)
{
    time_t cur_time;
    struct ring_buffer *ring_buf = (struct ring_buffer *)arg;
    while(1)
    {
    time(&cur_time);
    srand(cur_time);
    int seed = rand() % 11111;
    printf("******************************************\n");
    student_info *stu_info = get_student_info(cur_time + seed);
    printf("put a student info to ring buffer.\n");
    ring_buffer_put(ring_buf, (void *)stu_info, sizeof(student_info));
    printf("ring buffer length: %u\n", ring_buffer_len(ring_buf));
    printf("******************************************\n");
    usleep(100000);
    }
    return (void *)ring_buf;
}

pthread_t consumer_thread(void *arg)
{
    int err;
    pthread_t tid;
    err = pthread_create(&tid, NULL, unix_server, arg);
    if (err != 0)
    {
    fprintf(stderr, "Failed to create consumer thread.errno:%u, reason:%s\n",
        errno, strerror(errno));
    return -1;
    }
    return tid;
}
pthread_t producer_thread(void *arg)
{
    int err;
    pthread_t tid;
    err = pthread_create(&tid, NULL, unix_client, arg);
    if (err != 0)
    {
    fprintf(stderr, "Failed to create consumer thread.errno:%u, reason:%s\n",
        errno, strerror(errno));
    return -1;
    }
    return tid;
}


int main()
{
    void * buffer = NULL;
    uint32_t size = 0;
    pthread_t consume_pid, produce_pid;

    //pthread_mutex_t *f_lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t));
    //if (pthread_mutex_init(f_lock, NULL) != 0)
    //{
    //fprintf(stderr, "Failed init mutex,errno:%u,reason:%s\n",
    //    errno, strerror(errno));
    //return -1;
    //}
    //buffer = (void *)malloc(BUFFER_SIZE);
    //if (!buffer)
    //{
    //fprintf(stderr, "Failed to malloc memory.\n");
    //return -1;
    //}
    //size = BUFFER_SIZE;

    //struct ring_buffer *ring_buf = NULL;
    //ring_buf = ring_buffer_init(buffer, size, f_lock);

    //if (!ring_buf)
    //{
    //	fprintf(stderr, "Failed to init ring buffer.\n");
    //	return -1;
    //}

    printf("multi thread test.......\n");
    //produce_pid  = producer_thread((void*)ring_buf);
    //consume_pid  = consumer_thread((void*)ring_buf);
    consume_pid  = consumer_thread((void*)NULL);
	sleep(2);
    produce_pid  = producer_thread((void*)NULL);
    printf("producer_id:%d, consume_pid:%d\n", produce_pid, consume_pid);
    //pthread_join(produce_pid, NULL);
    //pthread_join(consume_pid, NULL);
    //ring_buffer_free(ring_buf);
    while(1)
    {
      sleep(100);
    }
    //free(f_lock);
    return 0;
}
