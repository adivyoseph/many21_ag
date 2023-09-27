#include <stddef.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "emq2.h"
emq2_t g_emq2;


void emq2_init(void) {

    g_emq2.head = 0;
   g_emq2.tail = 0;
    g_emq2.size = EMQ_FIFO_DEPTH_MAX; //size;
    //pthread_mutex_init(&p_q->lock, NULL);
    pthread_spin_init(&g_emq2.lock, PTHREAD_PROCESS_SHARED);
}


int emq2_available()
{
	if (g_emq2.tail < g_emq2.head)
		return g_emq2.head - g_emq2.tail - 1;
	else
		return g_emq2.head + (g_emq2.size - g_emq2.tail);
}


void emq2_write( emq_msg_t *p_msg){
    //pthread_mutex_lock(&p_q->lock);
    pthread_spin_lock(&g_emq2.lock);
    if (emq2_available() == 0) // when queue is full
    {
        printf("xQueue is full\n");
        p_msg->length = 0;
    }
    else
    {
        memcpy(&g_emq2.event[g_emq2.tail],p_msg, sizeof(emq_msg_t));
        //printf("=>%s write event[%d]", p_q->name, p_q->tail);
        (g_emq2.tail)++;
        g_emq2.write_cnt++;
        (g_emq2.tail) %= g_emq2.size;
        g_emq2.depth++;
    }
    //pthread_mutex_unlock(&p_q->lock);
    pthread_spin_unlock(&g_emq2.lock);
}   



void emq2_read(emq_msg_t *p_msg){
    //pthread_mutex_lock(&p_q->lock);
    pthread_spin_lock(&g_emq2.lock);
    if (g_emq2.head != g_emq2.tail){

        memcpy(p_msg,&g_emq2.event[g_emq2.head], sizeof(emq_msg_t));
        //printf("<=%s read event[%d]", p_q->name, p_q->head);
        (g_emq2.head)++;
        g_emq2.read_cnt++;
        (g_emq2.head) %= g_emq2.size;
        g_emq2.depth--;
    }
    else{
        p_msg->length = -1;
    }
   //pthread_mutex_unlock(&p_q->lock);
   pthread_spin_unlock(&g_emq2.lock);
}


   
