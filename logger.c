#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <strings.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>

#include "logger.h"

enum {sock_buf_size=41943040}; /* socket buffer size */

int keepRunning=1;

int str2int(const char*);

int getLogNumber(const char*);

void fillLog(Log*,time_t*, struct sockaddr_in*,int32_t);

void dispLog(Log*,unsigned char*);

void initNum(const char *,int);

void writeLog(FILE*,Log*,char*);

int str2int(const char* s){
/* this function is analog for atoi but no negative nums allowed, applied to parse port */
    int res = 0;
    if (*s == '-') {
        printf("error: negative port\n");
        exit(1);
    } /* TODO add check for *s */
    while (*s) {
        res = res*10+(*s-'0');
        s++;
    }
    return res;
}

void fillLog(Log * log,time_t* rTime,struct sockaddr_in* addr,int32_t pSize) {
/*fills Log struct */
    memset(log,0,sizeof(Log));
    log->label = label;
    log->cli_addr = addr->sin_addr.s_addr;
    log->cli_port = addr->sin_port;
    log->time = (int64_t)(*rTime);
    log->pSize = pSize;
}


void writeLog(FILE *log_file,Log *log, char* data) {
/* writes data to log file */
    fwrite(log,sizeof(Log),1,log_file);
    fwrite(data,log->pSize,1,log_file);
}

int getLogNumber(const char * file_num) {
/* get the actual log bumber to write */
    initNum(file_num,0);
    int num = 0;
    FILE *current_num = fopen(file_num,"r+b");
     if (current_num == NULL) {
         perror("Cant open file");
         exit(1);
     }
     fseek(current_num,0L,SEEK_END);
     int pos = ftell(current_num); /* chech if file is empty */
     fseek(current_num,0L,SEEK_SET); 

     if (pos==0) { 
        num++;
        fwrite(&num,sizeof(int),1,current_num);
     }
     else {
        fread(&num,sizeof(int),1,current_num);
        num++;
        fseek(current_num,0L,SEEK_SET); /*write at the beginning of the file */
        fwrite(&num,sizeof(int),1,current_num);
     }
     fclose(current_num);
     return num;
}

void initNum(const char * filename,int val) {
/* create file and write val to it */
    if(access(filename,F_OK)!=-1) {
        return;
    } else { 
        int s = val;
        FILE *f = fopen(filename,"wb");
        fwrite(&s,sizeof(int),1,f);
        fclose(f);
    }

}

void sigHandler() {
/* SIGTERM handler */
    printf(" Recieved SIGTERM\n");
    keepRunning=0;
}

void checkLogFile(char * filename,int log_number) {
    /* checks if log already exists, if so, find new log number */
    int changed = 0;
    while (access(filename,F_OK)!=-1) {
        if (!changed) changed = 1;
        sprintf(filename,"log_%07d.dat",++log_number);
    }
    if(changed) 
        initNum(filename,log_number);
}


int main(int argc, char** argv) {
    if(argc!=3) {
        printf("Usage: %s [port] [log_dir]",argv[0]);
        exit(1);
    }
    pid_t pid; 
    if((pid = fork())<0){
        perror("fork failed");
        exit(1);
    }
    else if(pid!=0){
        return 0; /* exit parent process */
    }

    time_t rTime; /* receive time */
    struct timeval  time_kernel;

    const char * file_num = "current.num"; /* file contains current number for log */
    const char * log_dir = argv[2];

    int sock;
    int32_t bytes_read;
    socklen_t addr_len;
    unsigned char recv_data[BUF_SIZE];
    int port = str2int(argv[1]);
    struct sockaddr_in server_addr , client_addr;
    Log in_log;

    char fname[28];

    struct sigaction sa; /* for signal handler */
    sa.sa_handler = sigHandler;
    sa.sa_flags = 0; // or SA_RESTART
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }


    struct msghdr   msg;
    struct iovec    iov;
    
    char            ctrl[CMSG_SPACE(sizeof(struct timeval))];
    struct cmsghdr *cmsg = (struct cmsghdr *) &ctrl;

    msg.msg_control      = (char *) ctrl;
    msg.msg_controllen   = sizeof(ctrl);    

    msg.msg_name         = &client_addr;
    msg.msg_namelen      = sizeof(client_addr);
    msg.msg_iov          = &iov;
    msg.msg_iovlen       = 1;
    iov.iov_base         = recv_data;
    iov.iov_len          = sizeof(recv_data);

    int log_number = getLogNumber(file_num);
    sprintf(fname,"log_%07d.dat",log_number);
    checkLogFile(fname,log_number);
    FILE *log_file = fopen(fname,"wb");

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Cant create socket");
        exit(1);
    }

    size_t newSize = sock_buf_size;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &newSize, sizeof(newSize)) == -1) {
    printf("cant");
    exit(1);
    }
    int timestampOn = 1;
    int rc;
    rc = setsockopt(sock, SOL_SOCKET, SO_TIMESTAMP, (int *) &timestampOn, sizeof(timestampOn)); /* enable kernel time for packets */
    if (0 > rc)
    {
        printf("setsockopt(SO_TIMESTAMP) failed.\n");
        exit(1);
    }
    

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    memset(&server_addr.sin_zero,0,sizeof(server_addr.sin_zero)); // this need to cast sockaddr_in to sockaddr 

    if (bind(sock,(struct sockaddr *)&server_addr,sizeof(struct sockaddr)) == -1) {
        perror("Cant bind");
        exit(1);
    }
    addr_len = sizeof(struct sockaddr);
    printf("\nUDP waiting for packets on port %d\n\n",port);
    fflush(stdout);
    int num=1,curr;
    while (keepRunning) {
      bytes_read = recvmsg(sock,&msg,0);
      if (bytes_read > 0) {
          /* standart macros for access control message */
        if (cmsg->cmsg_level == SOL_SOCKET &&
            cmsg->cmsg_type  == SCM_TIMESTAMP &&
            cmsg->cmsg_len   == CMSG_LEN(sizeof(time_kernel))) 
        {
            memcpy(&time_kernel, CMSG_DATA(cmsg), sizeof(time_kernel));
        }
      rTime = time_kernel.tv_sec;
      printf("readed %d\n at %s",bytes_read,ctime(&rTime));
      recv_data[bytes_read] = '\0'; /* ensure the ending*/
      fillLog(&in_log,&rTime,&client_addr,bytes_read);
      writeLog(log_file,&in_log,recv_data);  
      memcpy(&curr,recv_data,4);
        if(curr!=num) printf("corrupted at=%d\n",num);
      num++;
      }
      fflush(log_file);  
    }

    fclose(log_file);

    return 0;
}
