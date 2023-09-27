#ifndef __EMQ3_H__
#define __EMQ3_H__

#include "emq.h"

typedef struct {
    pthread_mutex_t lock; // __attribute__ ((aligned(CACHELINE_SIZE)));
    //thread_spinlock_t lock ;
    volatile int head ;
    volatile int tail ;
    emq_msg_t event[EMQ_FIFO_DEPTH_MAX+ 2] ;
    int depth;
    int size;
    int write_cnt;
    int read_cnt;
    char name[32];

} emq3_t ;



extern void     emq3_init(void);
extern void     emq3_write( emq_msg_t  *p_msg);
extern void      emq3_read(emq_msg_t  *p_msg);


#endif
