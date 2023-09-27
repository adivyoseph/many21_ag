#ifndef __EMQ_H__
#define __EMQ_H__




#ifndef CACHELINE_SIZE
#define CACHELINE_SIZE 64
#endif

#define EMQ_FIFO_DEPTH_MAX 0x80     //8*16


typedef struct emq_msg_s {
    int cmd;
    int src;
    int seq;
    int length;
    int data[256];
} emq_msg_t;


typedef struct {
    //pthread_mutex_t lock __attribute__ ((aligned(CACHELINE_SIZE)));
    pthread_spinlock_t lock ;
    volatile int head ;
    volatile int tail ;
    emq_msg_t event[EMQ_FIFO_DEPTH_MAX+ 2] ;
    int depth;
    int size;
    int write_cnt;
    int read_cnt;
    char name[32];

} emq_t ;



extern void     emq_init(void);
extern void     emq_write( emq_msg_t  *p_msg);
extern void      emq_read(emq_msg_t  *p_msg);


#endif
