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
#define IOGENTHREAD_MAX    2//16

typedef struct ioGenThreadContext_s {
    pthread_t   thread_id;
    int setaffinity;
    workq_t workq_in;
    workq_t workq_ack;
    int id;
} ioGenThreadContext_t;

ioGenThreadContext_t   g_contexts[IOGENTHREAD_MAX + 1];

typedef struct {
    void (*emq_init)(void);
    void (*emq_write)(emq_msg_t *p_msg);
    void (*emq_read)(emq_msg_t *p_msg);
} sharedq_t;

sharedq_t g_emqx;

void usage();
void *th_func(void *p_arg);
void *th_em(void *p_arg);



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


    for (i = 0; i <= IOGENTHREAD_MAX ; i++) {
        g_contexts[i].setaffinity = -1;
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
               g_contexts[IOGENTHREAD_MAX].setaffinity = atoi(optarg);
                printf("emulator cpu %d\n", g_contexts[IOGENTHREAD_MAX].setaffinity); 
                break; 


        case 't':                    //total send
                total_send = atoi(optarg);
                printf("total send %d\n", total_send); 
                break; 

        case 'a':                    //algorithym
                i= atoi(optarg);
                switch (i) {
                case 0: //emq spinlock not aligned
                    g_emqx.emq_init = emq_init;
                    g_emqx.emq_write= emq_write;
                    g_emqx.emq_read = emq_read;
                    printf("spinlock not aligned\n");
                    break;


                case 1: //emq spinlock aligned
                    g_emqx.emq_init = emq1_init;
                    g_emqx.emq_write= emq1_write;
                    g_emqx.emq_read = emq1_read;
                    printf("spinlock aligned\n");
                    break;

                case 2: //emq spinlock aligned only
                    g_emqx.emq_init = emq2_init;
                    g_emqx.emq_write= emq2_write;
                    g_emqx.emq_read = emq2_read;
                    printf("spinlock aligned only\n");
                    break;


                case 3: //emq mutex not aligned
                    g_emqx.emq_init = emq3_init;
                    g_emqx.emq_write= emq3_write;
                    g_emqx.emq_read = emq3_read;
                    printf("mutex not aligned\n");
                    break;


                case 4: //emq mutex aligned only
                    g_emqx.emq_init = emq4_init;
                    g_emqx.emq_write= emq4_write;
                    g_emqx.emq_read = emq4_read;
                    printf("mutex aligned only\n");
                    break;

                default:
                    break;

                }
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



    workq_init(&g_workq_cli, 16, "cli");

    for (i = 0; i <= IOGENTHREAD_MAX; i++) {
        sprintf(&work[0], "wq%d", i);
        workq_init(&g_contexts[i].workq_in, 16, &work[0]);

        sprintf(&work[0], "wq_ack%d", i);
        workq_init(&g_contexts[i].workq_ack, 16, &work[0]);
    }

//    signal(SIGCLD, SIG_IGN);
    for (i = 0;  i < IOGENTHREAD_MAX; i++) {
        g_contexts[i].id = i;
        pthread_create(&g_contexts[i].thread_id, NULL, th_func, (void *) &g_contexts[i]);
    }

    g_contexts[IOGENTHREAD_MAX].id = IOGENTHREAD_MAX;
     pthread_create(&g_contexts[IOGENTHREAD_MAX].thread_id, NULL, th_em, (void *) &g_contexts[IOGENTHREAD_MAX]);

     i = 0;
     while (1) {
            if(workq_read(&g_workq_cli, &msg)){
                if(msg.cmd == RSP_READY) {
                    i++;
                    printf("thread %d ready\n", msg.src);
                }
             if (i == ( ioGenThreads + 1)) {
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
return EXIT_SUCCESS;    printf("time  %lf %lf\n", accum1, accum );


    return 0;
}

void usage(){
    printf("-h     help\n-i     io_gen_threads 1-32\n-c      io_gen cpus x,y,z\n-s    cli_cpu\n-e   emulator cpu\n-t    total em msgs\n-a queu type\n");
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
    cpu_set_t my_set;        /* Define your cpu_set bit mask. */
     msg_t msg;
     emq_msg_t emq_msg;
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
            switch (msg.cmd) {
            case CMD_START:
                //clear stats
                send_cnt = msg.length;
                //printf("io_gen_%d started %d\n", this->id, send_cnt );
                break;

            default:
                break;
            }
        }
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
            g_emqx.emq_write(&emq_msg);
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
 * @brief emulator thread 
 * 
 * @param p_arg 
 * 
 * @return void* 
 */
void *th_em(void *p_arg){
    ioGenThreadContext_t *this = (ioGenThreadContext_t *) p_arg;
    //unsigned cpu, numa;
    cpu_set_t my_set;        /* Define your cpu_set bit mask. */
    msg_t msg;
    emq_msg_t emq_msg;
    workq_t *p_workqs[IOGENTHREAD_MAX];
    int i;

    //printf("Emulator  PID %d %d\n", getpid(), gettid());
    //build local completion queue look table
    for (i = 0; i < IOGENTHREAD_MAX; i++) {
        p_workqs[i] = &g_contexts[i].workq_ack;
    }

    CPU_ZERO(&my_set); 
    if (this->setaffinity >= 0) {
        CPU_SET(this->setaffinity, &my_set);
        sched_setaffinity(0, sizeof(cpu_set_t), &my_set);
    }

    msg.cmd = RSP_READY;
    msg.src = 16;
    msg.length = 0;
    if(workq_write(&g_workq_cli, &msg)){
        printf("%d q is full\n", this->id);
    }

    while (1){
//            if(workq_read(&this->workq_in, &msg)){
//           }
        g_emqx.emq_read(&emq_msg);
        if (emq_msg.length > 0 ) {
            //do something


            //ack
            if(p_workqs[emq_msg.src] ){
                msg.cmd = EM_RSP_ACK;
                msg.src = 16;
                msg.length = emq_msg.length;
                if(workq_write(p_workqs[emq_msg.src] , &msg)){
                    printf("em to ack %d  is full\n", emq_msg.src);
                 }
            }

        
        }
    }
}



