#include <stddef.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "emq.h"
emq_t g_emq;


void emq_init(void) {

    g_emq.head = 0;
   g_emq.tail = 0;
    g_emq.size = EMQ_FIFO_DEPTH_MAX; //size;
    //pthread_mutex_init(&p_q->lock, NULL);
    pthread_spin_init(&g_emq.lock, PTHREAD_PROCESS_SHARED);
}


int emq_available()
{
	if (g_emq.tail < g_emq.head)
		return g_emq.head - g_emq.tail - 1;
	else
		return g_emq.head + (g_emq.size - g_emq.tail);
}


void emq_write( emq_msg_t *p_msg){
    //pthread_mutex_lock(&p_q->lock);
    pthread_spin_lock(&g_emq.lock);
    if (emq_available() == 0) // when queue is full
    {
        printf("xQueue is full\n");
        p_msg->length = 0;
    }
    else
    {
        memcpy(&g_emq.event[g_emq.tail],p_msg, sizeof(emq_msg_t));
        //printf("=>%s write event[%d]", p_q->name, p_q->tail);
        (g_emq.tail)++;
        g_emq.write_cnt++;
        (g_emq.tail) %= g_emq.size;
        g_emq.depth++;
    }
    //pthread_mutex_unlock(&p_q->lock);
    pthread_spin_unlock(&g_emq.lock);
}   



void emq_read(emq_msg_t *p_msg){
    //pthread_mutex_lock(&p_q->lock);
    pthread_spin_lock(&g_emq.lock);
    if (g_emq.head != g_emq.tail){

        memcpy(p_msg,&g_emq.event[g_emq.head], sizeof(emq_msg_t));
        //printf("<=%s read event[%d]", p_q->name, p_q->head);
        (g_emq.head)++;
        g_emq.read_cnt++;
        (g_emq.head) %= g_emq.size;
        g_emq.depth--;
    }
    else{
        p_msg->length = -1;
    }
   //pthread_mutex_unlock(&p_q->lock);
   pthread_spin_unlock(&g_emq.lock);
}


   
