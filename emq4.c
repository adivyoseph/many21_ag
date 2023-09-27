#include <stddef.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "emq4.h"
emq4_t g_emq4;


void emq4_init(void) {

    g_emq4.head = 0;
   g_emq4.tail = 0;
    g_emq4.size = EMQ_FIFO_DEPTH_MAX; //size;
    pthread_mutex_init(&g_emq4.lock, NULL);
    //pthread_spin_init(&g_emq4.lock, PTHREAD_PROCESS_SHARED);
}


int emq4_available()
{
	if (g_emq4.tail < g_emq4.head)
		return g_emq4.head - g_emq4.tail - 1;
	else
		return g_emq4.head + (g_emq4.size - g_emq4.tail);
}


void emq4_write( emq_msg_t *p_msg){
    pthread_mutex_lock(&g_emq4.lock);
    //pthread_spin_lock(&g_emq4.lock);
    if (emq4_available() == 0) // when queue is full
    {
        printf("xQueue is full\n");
        p_msg->length = 0;
    }
    else
    {
        memcpy(&g_emq4.event[g_emq4.tail],p_msg, sizeof(emq_msg_t));
        //printf("=>%s write event[%d]", p_q->name, p_q->tail);
        (g_emq4.tail)++;
        g_emq4.write_cnt++;
        (g_emq4.tail) %= g_emq4.size;
        g_emq4.depth++;
    }
    pthread_mutex_unlock(&g_emq4.lock);
    //pthread_spin_unlock(&g_emq4.lock);
}   



void emq4_read(emq_msg_t *p_msg){
    pthread_mutex_lock(&g_emq4.lock);
    //pthread_spin_lock(&g_emq4.lock);
    if (g_emq4.head != g_emq4.tail){

        memcpy(p_msg,&g_emq4.event[g_emq4.head], sizeof(emq_msg_t));
        //printf("<=%s read event[%d]", p_q->name, p_q->head);
        (g_emq4.head)++;
        g_emq4.read_cnt++;
        (g_emq4.head) %= g_emq4.size;
        g_emq4.depth--;
    }
    else{
        p_msg->length = -1;
    }
   pthread_mutex_unlock(&g_emq4.lock);
   //pthread_spin_unlock(&g_emq4.lock);
}


   
