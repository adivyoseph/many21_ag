#define _GNU_SOURCE
#include <assert.h>
#include <sched.h> /* getcpu */
#include <stdio.h> 
#include <stdlib.h> 
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>   
#include <pthread.h> 
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <sys/syscall.h>

#include "workq.h"
#include "emq.h"
#include "emq1.h"
#include "emq2.h"
#include "emq3.h"
#include "emq4.h"

#define BILLION  1000000000L;
#define IOGENTHREAD_MAX    16

typedef struct ioGenThreadContext_s {
    pthread_t   thread_id;
    int setaffinity;
    workq_t workq_in;
    workq_t workq_ack;
    emq_t   emq;
    int id;
    int state;
} ioGenThreadContext_t;

ioGenThreadContext_t   g_contexts[IOGENTHREAD_MAX + 2];
#define THREAD_AG     (IOGENTHREAD_MAX)
#define THREAD_EM      (IOGENTHREAD_MAX + 1)

typedef struct {
    void (*emq_init)(emq_t  *p_emq);
    void (*emq_write)emq_t  *p_emq, (emq_msg_t *p_msg);
    void (*emq_read)(emq_t  *p_emq, emq_msg_t *p_msg);
} sharedq_t;

sharedq_t g_emqx;

void usage();
void *th_func(void *p_arg);
void *th_em(void *p_arg);
void *th_ag(void *p_arg);



#define CMD_START 1
#define CMD_STOP    2
#define CMD_CLEAR  3
#define RSP_READY     1
#define RSP_DONE     2

#define EM_RSP_ACK     1
#define IOGEN_EM_REQ_MAX 8

workq_t g_workq_cli;
/**
 * 
 * 
 * @author martin (9/25/23)
 * @brief start
 * @param argc 
 * @param argv 
 * 
 * @return int 
 */
int main(int argc, char **argv) {
    int opt;
    int ioGenThreads = 4;
    int setaffinity  = -1;


    int i, j, k;
    unsigned cpu, numa;
    char work[64];
    char cwork[64];
    cpu_set_t my_set;        /* Define your cpu_set bit mask. */

    msg_t msg;
    int total_sent = 0;
    struct timespec start;
    struct timespec end;
    double accum, accum1;
    int total_send = 1000000;
    int msgsPerIOGen = 0;
    int first_count = 0;


    for (i = 0; i < THREAD_EM ; i++) {
        g_contexts[i].setaffinity = -1;
        g_contexts[i].id = i;
    }

    //emq set default
    g_emqx.emq_init = emq_init;
    g_emqx.emq_write= emq_write;
    g_emqx.emq_read = emq_read;


    getcpu(&cpu, &numa);
    printf("CLI %u %u\n", cpu, numa);
      
    while((opt = getopt(argc, argv, "hi:c:s:e:t:a:")) != -1) 
    { 
        switch(opt) 
        { 
        case 'h':                   //help
            usage();
            return 0;
            break;

        case 'i':                //threads for iogen
                ioGenThreads = atoi(optarg);
                if (ioGenThreads >  IOGENTHREAD_MAX) {
                    ioGenThreads  =  IOGENTHREAD_MAX;
                }
                printf("io_gen_threads: %s\n", optarg);
                break; 

        case 'c':            //cpus for iogen threads
                printf("cpus: %s \n", optarg); 
                //coma seperated list 
                strcpy(cwork, optarg);
                i = 0;
                j = 0;
                k = 0;
                while (cwork[i] != 0) {
                    if (cwork[i] == ' ') {
                        i++;
                        continue;
                    }
                    while ((cwork[i] >= '0') && (cwork[i] <= '9')) {
                        work[j] = cwork[i];
                        work[j+1] = '\0';
                        i++;
                        j++;
                    }
                    if(cwork[i] == '\0'){
                        printf("pin iogen %2d to %s\n", k, work);
                        g_contexts[k].setaffinity = atoi(work);
                    }
                    if (cwork[i] == ',') {
                        printf("pin iogen %2d to %s\n", k, work);
                        g_contexts[k].setaffinity = atoi(work);
                        k++;
                        if (k >=  IOGENTHREAD_MAX) {
                            break;
                        }
                        j = 0;
                        i++;
                    }
                }
                break; 

        case 's':                    //cli cpu mapping
                setaffinity = atoi(optarg);
                printf("cli cpu %d\n", setaffinity); 
                break; 


        case 'e':                    //emulator  cpu mapping
               g_contexts[THREAD_EM].setaffinity = atoi(optarg);
                printf("emulator cpu %d\n", g_contexts[THREAD_EM.setaffinity); 
                break; 


        case 't':                    //total send
                total_send = atoi(optarg);
                printf("total send %d\n", total_send); 
                break; 

        case 'a':                    //aggegrator cpu
            g_contexts[THREAD_AG].setaffinity = atoi(optarg);
             printf("aggregator cpu %d\n", g_contexts[THREAD_AG.setaffinity); 
             break; 

  
        default:
            usage();
            return 0;
                break;

        } 
    } 

    printf("\n");

    g_emqx.emq_init();

    CPU_ZERO(&my_set); 
    if (setaffinity >= 0) {
        CPU_SET(setaffinity, &my_set);
        sched_setaffinity(0, sizeof(cpu_set_t), &my_set);
    }

    getcpu(&cpu, &numa);
    printf("CLI %u %u\n", cpu, numa);



    workq_init(&g_workq_cli);

    for (i = 0; i < IOGENTHREAD_MAX; i++) {
        if (i <  ioGenThreads) {
            g_contexts[i].state = 1;
        }
        else {
            g_contexts[i].state = 0;
        }
        workq_init(&g_contexts[i].workq_in);
        workq_init(&g_contexts[i].workq_ack);
        g_emqx.emq_init(&g_contexts[i].emq);
    }

//    signal(SIGCLD, SIG_IGN);
    for (i = 0;  i < IOGENTHREAD_MAX; i++) {
        g_contexts[i].id = i;
        pthread_create(&g_contexts[i].thread_id, NULL, th_func, (void *) &g_contexts[i]);
    }


    g_contexts[THREAD_AG].id = THREAD_AG;
     g_emqx.emq_init(&g_contexts[THREAD_AG].emq);
     pthread_create(&g_contexts[THREAD_AG].thread_id, NULL, th_ag, (void *) &g_contexts[THREAD_AG]);


     g_contexts[THREAD_EM].id = THREAD_EM;
     pthread_create(&g_contexts[THREAD_EM].thread_id, NULL, th_em, (void *) &g_contexts[THREAD_EM]);

     i = 0;
     while (1) {
            if(workq_read(&g_workq_cli, &msg)){
                if(msg.cmd == RSP_READY) {
                    i++;
                    printf("thread %d ready\n", msg.src);
                }
             if (i  >= THREAD_EM) {
                 break;
             }
         }
     }

     printf("all threads ready\n");

     if (ioGenThreads == 1) {
          first_count = total_send;
     }
     else {
         msgsPerIOGen = (int)(total_send / ioGenThreads);
         first_count = total_send - (msgsPerIOGen* (ioGenThreads-1));
     }
     //start run
     msg.cmd = CMD_START;
     clock_gettime(CLOCK_REALTIME, &start);
      for (i = 0;  i < ioGenThreads; i++) {
          if (i == 0) {
              msg.length = first_count;
          }
          else {
              msg.length = msgsPerIOGen;
          }
           if(workq_write(&g_contexts[i].workq_in, &msg)){
               printf("%d q is full\n", i);
              }
              else{
               total_sent += msg.length;
              }
          }
   

    i = 0;
    while (1) {

        if(workq_read(&g_workq_cli, &msg)){
            if(msg.cmd == RSP_DONE) {
                i++;
                //printf("thread %d done\n", msg.src);
            }
            if (i == ( ioGenThreads )) {
             break;
           }
        }
    }

    clock_gettime(CLOCK_REALTIME, &end);
    printf("finished total sent %d\n", total_sent);
    accum = ( end.tv_sec - start.tv_sec ) + (double)( end.tv_nsec - start.tv_nsec ) / (double)BILLION;
    printf( "%lf\n", accum );
    return 0;
}

void usage(){
    printf("-h     help\n-i     io_gen_threads 1-32\n-c      io_gen cpus x,y,z\n-s    cli_cpu\n-e   emulator cpu\n-t    total em msgs\n-a aggregator cpu\n");
}


/**
 * 
 * 
 * @author martin (9/25/23) 
 *  
 * @brief io gen thread 
 * 
 * @param p_arg 
 * 
 * @return void* 
 */
void *th_func(void *p_arg){
    ioGenThreadContext_t *this = (ioGenThreadContext_t *) p_arg;
    //unsigned cpu, numa;
    cpu_set_t           my_set;        /* Define your cpu_set bit mask. */
     msg_t                  msg;
     emq_msg_t      emq_msg;
     int send_cnt = 0;
     int emOutstandingRequests = 0;

    //printf("Thread_%d PID %d %d\n", this->id, getpid(), gettid());


    CPU_ZERO(&my_set); 
    if (this->setaffinity >= 0) {
        CPU_SET(this->setaffinity, &my_set);
        sched_setaffinity(0, sizeof(cpu_set_t), &my_set);
    }

    msg.cmd = RSP_READY;
    msg.src = this->id;
    msg.length = 0;
    if(workq_write(&g_workq_cli, &msg)){
        printf("%d q is full\n", this->id);
    }

    while (1){
        //look for CLI command request
        if(workq_read(&this->workq_in, &msg)){
           if(msg.cmd == CMD_START){
                //clear stats
                send_cnt = msg.length;
                //printf("io_gen_%d started %d\n", this->id, send_cnt );
                break;
            }
        }
    }

    while (1) {
        //track emulator ack window to meter new requests
        if(workq_read(&this->workq_ack, &msg)){
            if (msg.cmd == EM_RSP_ACK) {
                if (emOutstandingRequests) {
                    emOutstandingRequests --;
                }
            }
        }

        if ((send_cnt > 0)&& (emOutstandingRequests < IOGEN_EM_REQ_MAX)) {
             //send a work item
            emq_msg.src = this->id;
            emq_msg.seq = send_cnt;
            emq_msg.length = 1;
            g_emqx.emq_write(&this->emq, &emq_msg);
            if (emq_msg.length ) {
                emOutstandingRequests++;
                send_cnt--;
                if (send_cnt == 0) {
                   msg.cmd = RSP_DONE;
                   msg.src = this->id;
                   msg.length = 0;
                   if(workq_write(&g_workq_cli, &msg)){
                       printf("%d q is full\n", this->id);
                       exit(10);
                    }
                }     
            }
        }
    }
}

/**
 * 
 * 
 * @author martin (9/25/23) 
 *  
 * @brief aggregator  thread 
 * 
 * @param p_arg 
 * 
 * @return void* 
 */
void *th_ag(void *p_arg){
    ioGenThreadContext_t *this = (ioGenThreadContext_t *) p_arg;
    //unsigned cpu, numa;
    cpu_set_t          my_set;        /* Define your cpu_set bit mask. */
    msg_t                 msg;
    emq_msg_t     emq_msg;
    emq_t                *p_emqs[IOGENTHREAD_MAX];
    int i;

    //printf("Emulator  PID %d %d\n", getpid(), gettid());
    //build local completion queue look table
    for (i = 0; i < IOGENTHREAD_MAX; i++) {
        if (g_contexts[i].state == 1) {
            p_emqs[i] = &g_contexts[i].emq;
        }
        else{
             p_emqs[i] = NULL:
        }
    }

    CPU_ZERO(&my_set); 
    if (this->setaffinity >= 0) {
        CPU_SET(this->setaffinity, &my_set);
        sched_setaffinity(0, sizeof(cpu_set_t), &my_set);
    }

    msg.cmd = RSP_READY;
    msg.src = this->id;
    msg.length = 0;
    if(workq_write(&g_workq_cli, &msg)){
        printf("%d q is full\n", this->id);
    }

    i = 0;
    while (1){
        if (p_emqs[i] != NULL) {
            g_emqx.emq_read(p_emqs[i] , &emq_msg);
            if (emq_msg.length > 0) {
                 g_emqx.emq_write(&this->emq, &emq_msg);
                 if (emq_msg.length == 0) {
                 }
            }
        }
        i++;
        if (i >=  IOGENTHREAD_MAX ) {
            i = 0;
        }
    }
}



 /**
 * 
 * 
 * @author martin (9/25/23) 
 *  
 * @brief emulator thread 
 * 
 * @param p_arg 
 * 
 * @return void* 
 */
void *th_em(void *p_arg){
    ioGenThreadContext_t *this = (ioGenThreadContext_t *) p_arg;
    //unsigned cpu, numa;
    cpu_set_t         my_set;        /* Define your cpu_set bit mask. */
    msg_t                msg;
    emq_msg_t     emq_msg;
    emq_t *p_workq = g_contexts[THREAD_AG].workq_req;
    workq_t *p_ackqs[IOGENTHREAD_MAX];
    workq_t *p_workq_ack;
    int i;


    for (i = 0; i < IOGENTHREAD_MAX; i++) {
            p_ackqs[i] = &g_contexts[i].workq_ack;
    }

    //printf("Emulator  PID %d %d\n", getpid(), gettid());

    CPU_ZERO(&my_set); 
    if (this->setaffinity >= 0) {
        CPU_SET(this->setaffinity, &my_set);
        sched_setaffinity(0, sizeof(cpu_set_t), &my_set);
    }

    msg.cmd = RSP_READY;
    msg.src = this->id;
    msg.length = 0;
    if(workq_write(&g_workq_cli, &msg)){
        printf("%d q is full\n", this->id);
    }

    while (1){
          g_emqx.emq_read(p_workq, &emq_msg);
           if (emq_msg.length > 0) {
               //do something


               //ack
             p_workq_ack = p_workqs[emq_msg.src];
            if( p_workq_ack ){
                msg.cmd = EM_RSP_ACK;
                msg.src = this->id;
                msg.length = emq_msg.length;
                if(workq_write(p_workq_ack  , &msg)){
                    printf("em to ack is full\n");
                 }
            }
        }
    }
}



