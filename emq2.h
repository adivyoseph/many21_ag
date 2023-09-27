#ifndef __EMQ2_H__
#define __EMQ2_H__

#include "emq.h"



typedef struct {
    //pthread_mutex_t lock __attribute__ ((aligned(CACHELINE_SIZE)));
    pthread_spinlock_t lock __attribute__ ((aligned(CACHELINE_SIZE)));
    volatile int head ;
    volatile int tail ;
    emq_msg_t event[EMQ_FIFO_DEPTH_MAX+ 2] ;
    int depth;
    int size;
    int write_cnt;
    int read_cnt;
    char name[32];

} emq2_t __attribute__ ((aligned(CACHELINE_SIZE)));



extern void     emq2_init(void);
extern void     emq2_write( emq_msg_t  *p_msg);
extern void      emq2_read(emq_msg_t  *p_msg);


#endif
