#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

#define BUF_SIZE 1500

#define USR0 "dsdsds"

#define START_CMD "START"
#define STOP_CMD "STOP"

const char * logger_pidfile = "/tmp/logger.pid";
const char * log_dir = "logs";

const char * heartbleed_led_trigger = "/sys/devices/leds.7/leds/beaglebone:green:heartbeat/trigger";
const char * heartbleed_led_delayon = "/sys/devices/leds.7/leds/beaglebone:green:heartbeat/delay_on";
const char * heartbleed_led_delayoff = "/sys/devices/leds.7/leds/beaglebone:green:heartbeat/delay_off";


void write_to_sysfs(const char * path,const char * buf) {
    int fd = open(path, O_WRONLY);
    write(fd, buf, strlen(buf));
    close(fd);
}

void setup_leds(int isWaiting) {
    int fd;
    char * state = "none";
    char * delay;
    if(isWaiting) {
        delay = "1000";
    } else {
        delay = "100";
    }
    write_to_sysfs(heartbleed_led_trigger,  state);
    write_to_sysfs(heartbleed_led_delayon,  delay);
    write_to_sysfs(heartbleed_led_delayoff, delay);
}


enum {
    logger_port = 9090,
    runner_port = 9999 
};


void blink(char * device, int freq) {

}

int read_pid(const char * fname) {
    int pid;
    FILE *f = fopen(fname, "r");
    if(f == NULL) {
        perror("PID file missing");
        return -1;
    }
    fscanf(f, "%d", &pid);
    fclose(f);
    return pid;
}

int main(int argc, char**argv)
{
   setup_leds(0);

   char run_str[100];
   int sockfd,n;
   struct sockaddr_in servaddr,cliaddr;
   socklen_t len;
   char mesg[BUF_SIZE];

   struct stat st = {0};

   if (stat(log_dir, &st) == -1) {
       mkdir(log_dir, 0700);
   }    

   sockfd=socket(AF_INET,SOCK_DGRAM,0);

   memset((char*)&servaddr,0,sizeof(servaddr));
   servaddr.sin_family = AF_INET;
   servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
   servaddr.sin_port=htons(runner_port);
   if(bind(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr)) == -1) {
       perror("Cant bind");
       return 1;
   }

   sprintf(run_str, "./logger %d %s",logger_port, log_dir);


   while (1)
   {
      len = sizeof(cliaddr);
      n = recvfrom(sockfd,mesg, BUF_SIZE ,0 ,(struct sockaddr *)&cliaddr,&len);
      mesg[n] = '\0';
      if(strcmp(mesg,START_CMD) == 0) {
         printf("Starting....\n");
         system(run_str);
         setup_leds(1);
      }
      if(strcmp(mesg,STOP_CMD) == 0) {
         printf("Stopping....\n");
         int pid = read_pid(logger_pidfile);
         printf("Killing PID %d ....\n",pid);
         if(pid > 0) {
            kill(pid,2);
         }
         setup_leds(0);
      }
      fflush(NULL);
   }
}