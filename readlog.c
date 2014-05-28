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

#include "logger.h"


void dispLog(Log* log,unsigned char *buf){
 /* just displays the Log struct */
    struct in_addr *t;
    size_t i = 0;
    t = (struct in_addr *)(&log->cli_addr); //dirty cast
    printf("-------------\n");
    printf("Packet recevied from %s:%d at %s",inet_ntoa(*t),ntohs(log->cli_port),ctime((time_t*)&(log->time)));
   // printf("Contents:\n%s\n",log->buf);
    printf("-------------\n");
    for(;i<log->pSize;i++){
         printf("0x%02x ",buf[i]);
     }
    printf("\n");
    fflush(stdout);

}

void readLog(const char* filename){
/* reads log */
    Log log;
    uint32_t current;
    uint32_t size;
    unsigned char buf[BUF_SIZE];
    uint32_t num=0,cnum;
    size_t i = 0;
    FILE *f = fopen(filename,"rb");
    if(f==NULL) {
        perror("Cant open file");
        exit(1);
    }
    while(fread(&log,sizeof(Log),1,f)!=0){
        memset(buf,0,sizeof(buf)); // ? needed?
        if(log.label != label) {
             printf("File %s corrupted",filename);
             exit(1);
        }
        fread(buf,log.pSize,1,f);
    //    dispLog(&log,buf);
        printf("\n");
        memcpy(&cnum,buf,4);/* fill uint32_t with packet number */
        if((cnum-num)!=1) {
              printf("missed packed number: %d\n",num);   
            //  break;
         }
        num++;
    }
    fclose(f);
}

int main(int argc, const char *argv[])
{
    if(argc!=2) {
        printf("Usage [%s] [log_name]",argv[0]);
        exit(1);
    }
    readLog(argv[1]);
    return 0;
}
