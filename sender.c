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

enum ll { max=0x1337 };

int a[max];
int main(int argc, char **argv)
{
  if (argc != 5) {
    puts("usage: send ipaddr port data_count data_size");
    exit(1);
  }
  int s;
  int ret;
  int Num=1;
  int maxNum=atoi(argv[3]);
  size_t size = atoi(argv[4]);
  unsigned char* data;
  struct sockaddr_in addr;
  struct timespec tim, tim2;
  tim.tv_sec = 0;
  tim.tv_nsec = 500; 
  data = (unsigned char*)malloc(size);
  memset(data,0,sizeof(data));
  addr.sin_family = AF_INET;
  ret = inet_aton(argv[1], &addr.sin_addr);
  if (ret == 0) { perror("inet_aton"); exit(1); }
  addr.sin_port = htons(atoi(argv[2]));
 
  s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s == -1) { perror("socket"); exit(1); }
 while(Num<=maxNum){
     memcpy(data,&Num,sizeof(Num));
     printf("sending %d in size %d \n ",Num,size);
     ret = sendto(s, data, size, 0, (struct sockaddr *)&addr, sizeof(addr));
     if (ret == -1) { perror("sendto"); exit(1); }
     Num++;
 //nanosleep(&tim,&tim2);
 }
 
  return 0;
}
