#include <stddef.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "emq3.h"
emq3_t g_emq3;


void emq3_init(void) {

    g_emq3.head = 0;
   g_emq3.tail = 0;
    g_emq3.size = EMQ_FIFO_DEPTH_MAX; //size;
    pthread_mutex_init(&g_emq3.lock, NULL);
    //pthread_spin_init(&g_emq3.lock, PTHREAD_PROCESS_SHARED);
}


int emq3_available()
{
	if (g_emq3.tail < g_emq3.head)
		return g_emq3.head - g_emq3.tail - 1;
	else
		return g_emq3.head + (g_emq3.size - g_emq3.tail);
}


void emq3_write( emq_msg_t *p_msg){
    pthread_mutex_lock(&g_emq3.lock);
    //pthread_spin_lock(&g_emq3.lock);
    if (emq3_available() == 0) // when queue is full
    {
        printf("xQueue is full\n");
        p_msg->length = 0;
    }
    else
    {
        memcpy(&g_emq3.event[g_emq3.tail],p_msg, sizeof(emq_msg_t));
        //printf("=>%s write event[%d]", p_q->name, p_q->tail);
        (g_emq3.tail)++;
        g_emq3.write_cnt++;
        (g_emq3.tail) %= g_emq3.size;
        g_emq3.depth++;
    }
    pthread_mutex_unlock(&g_emq3.lock);
    //pthread_spin_unlock(&g_emq3.lock);
}   



void emq3_read(emq_msg_t *p_msg){
    pthread_mutex_lock(&g_emq3.lock);
    //pthread_spin_lock(&g_emq3.lock);
    if (g_emq3.head != g_emq3.tail){

        memcpy(p_msg,&g_emq3.event[g_emq3.head], sizeof(emq_msg_t));
        //printf("<=%s read event[%d]", p_q->name, p_q->head);
        (g_emq3.head)++;
        g_emq3.read_cnt++;
        (g_emq3.head) %= g_emq3.size;
        g_emq3.depth--;
    }
    else{
        p_msg->length = -1;
    }
   pthread_mutex_unlock(&g_emq3.lock);
   //pthread_spin_unlock(&g_emq3.lock);
}


   
