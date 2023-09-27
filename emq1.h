#ifndef __EMQ1_H__
#define __EMQ1_H__

#include "emq.h"



typedef struct {
    //pthread_mutex_t lock __attribute__ ((aligned(CACHELINE_SIZE)));
    pthread_spinlock_t lock __attribute__ ((aligned(CACHELINE_SIZE)));
    volatile int head __attribute__ ((aligned(CACHELINE_SIZE)));
    volatile int tail __attribute__ ((aligned(CACHELINE_SIZE)));
    emq_msg_t event[EMQ_FIFO_DEPTH_MAX+ 2] __attribute__ ((aligned(CACHELINE_SIZE)));
    int depth;
    int size;
    int write_cnt;
    int read_cnt;
    char name[32];

} emq1_t __attribute__ ((aligned(CACHELINE_SIZE)));



extern void     emq1_init(void);
extern void     emq1_write( emq_msg_t  *p_msg);
extern void      emq1_read(emq_msg_t  *p_msg);


#endif
