#include <stddef.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "emq1.h"
emq1_t g_emq1;


void emq1_init(void) {

    g_emq1.head = 0;
   g_emq1.tail = 0;
    g_emq1.size = EMQ_FIFO_DEPTH_MAX; //size;
    //pthread_mutex_init(&p_q->lock, NULL);
    pthread_spin_init(&g_emq1.lock, PTHREAD_PROCESS_SHARED);
}


int emq1_available()
{
	if (g_emq1.tail < g_emq1.head)
		return g_emq1.head - g_emq1.tail - 1;
	else
		return g_emq1.head + (g_emq1.size - g_emq1.tail);
}


void emq1_write( emq_msg_t *p_msg){
    //pthread_mutex_lock(&p_q->lock);
    pthread_spin_lock(&g_emq1.lock);
    if (emq1_available() == 0) // when queue is full
    {
        printf("xQueue is full\n");
        p_msg->length = 0;
    }
    else
    {
        memcpy(&g_emq1.event[g_emq1.tail],p_msg, sizeof(emq_msg_t));
        //printf("=>%s write event[%d]", p_q->name, p_q->tail);
        (g_emq1.tail)++;
        g_emq1.write_cnt++;
        (g_emq1.tail) %= g_emq1.size;
        g_emq1.depth++;
    }
    //pthread_mutex_unlock(&p_q->lock);
    pthread_spin_unlock(&g_emq1.lock);
}   



void emq1_read(emq_msg_t *p_msg){
    //pthread_mutex_lock(&p_q->lock);
    pthread_spin_lock(&g_emq1.lock);
    if (g_emq1.head != g_emq1.tail){

        memcpy(p_msg,&g_emq1.event[g_emq1.head], sizeof(emq_msg_t));
        //printf("<=%s read event[%d]", p_q->name, p_q->head);
        (g_emq1.head)++;
        g_emq1.read_cnt++;
        (g_emq1.head) %= g_emq1.size;
        g_emq1.depth--;
    }
    else{
        p_msg->length = -1;
    }
   //pthread_mutex_unlock(&p_q->lock);
   pthread_spin_unlock(&g_emq1.lock);
}


   
