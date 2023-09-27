#include <stddef.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "emq.h"



void emq_init(emq_t  *p_emq {
    p_emq->head = 0;
   p_emq->tail = 0;
    pthread_spin_init(&p_emq->lock, PTHREAD_PROCESS_SHARED);
}


inline int emq_available(emq_t  *p_emq )
{
	if (p_emq->.tail < p_emq->head)
		return p_emq->head - p_emq->tail - 1;
	else
		return p_emq->head + (EMQ_FIFO_DEPTH_MAX - p_emq->.tail);
}


void emq_write( emq_t  *p_emq , emq_msg_t *p_msg){
    //pthread_mutex_lock(&p_q->lock);
    pthread_spin_lock(&p_emq->lock);
    if (emq_available() == 0) // when queue is full
    {
        //printf("xQueue is full\n");
        p_msg->length = 0;
    }
    else
    {
       // memcpy(&p_emq->event[p_emq->tail],p_msg, sizeof(emq_msg_t));
        p_emq->event[p_emq->tail].cmd        = p_msg.cmd;
        p_emq->event[p_emq->tail].src           = p_msg.src;
        p_emq->event[p_emq->tail].length    = p_msg.length;
        //printf("=>%s write event[%d]", p_q->name, p_q->tail);
        (p_emq->tail)++;
        (p_emq->tail) %= EMQ_FIFO_DEPTH_MAX;
    }
    //pthread_mutex_unlock(&p_q->lock);
    pthread_spin_unlock(&p_emq->lock);
}   



void emq_read(emq_t  *p_emq , emq_msg_t *p_msg){
    //pthread_mutex_lock(&p_q->lock);
    pthread_spin_lock(&p_emq->lock);
    if (p_emq->head != p_emq->tail){

        //memcpy(p_msg,&p_emq->event[p_emq->head], sizeof(emq_msg_t));
        p_msg.cmd =           p_emq->event[p_emq->head].cmd       ;
        p_msg.src =             p_emq->event[p_emq->head].src           ;
        p_msg.length =      p_emq->event[p_emq->head].length   ;
        //printf("<=%s read event[%d]", p_q->name, p_q->head);
        (p_emq->head)++;
        (p_emq->head) %= g_EMQ_FIFO_DEPTH_MAX;
    }
    else{
        p_msg->length = -1;
    }
   //pthread_mutex_unlock(&p_q->lock);
   pthread_spin_unlock(&p_emq->lock);
}


   
