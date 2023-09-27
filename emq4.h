#ifndef __EMQ4_H__
#define __EMQ4_H__

#include "emq.h"

typedef struct {
    pthread_mutex_t lock  __attribute__ ((aligned(CACHELINE_SIZE)));
    //thread_spinlock_t lock ;
    volatile int head ;
    volatile int tail ;
    emq_msg_t event[EMQ_FIFO_DEPTH_MAX+ 2] ;
    int depth;
    int size;
    int write_cnt;
    int read_cnt;
    char name[32];

} emq4_t __attribute__ ((aligned(CACHELINE_SIZE)));



extern void     emq4_init(void);
extern void     emq4_write( emq_msg_t  *p_msg);
extern void      emq4_read(emq_msg_t  *p_msg);


#endif
