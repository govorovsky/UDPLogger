#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>

#define BUF_SIZE 1500

#define START_CMD "START"
#define STOP_CMD "STOP"
#define SHUTDOWN_CMD "POWEROFF"

const char * logger_pidfile = "/tmp/logger.pid";
const char * manager_pidfile = "/tmp/manager.pid";
const char * log_dir = "/var/udp_logs";

const char * heartbleed_led_trigger = "/sys/devices/leds.7/leds/beaglebone:green:heartbeat/trigger";
const char * mmc_led_trigger = "/sys/devices/leds.7/leds/beaglebone:green:mmc0/trigger";
const char * usr2_led_trigger = "/sys/devices/leds.7/leds/beaglebone:green:usr2/trigger";
const char * usr3_led_trigger = "/sys/devices/leds.7/leds/beaglebone:green:usr3/trigger";
const char * heartbleed_led_delayon = "/sys/devices/leds.7/leds/beaglebone:green:heartbeat/delay_on";
const char * heartbleed_led_delayoff = "/sys/devices/leds.7/leds/beaglebone:green:heartbeat/delay_off";

enum {
    logger_port = 9090,
    runner_port = 9999 
};

enum {
    LOGGING, WAITING
};

void write_to_sysfs(const char * path,const char * buf) {
    int fd = open(path, O_WRONLY);
    write(fd, buf, strlen(buf));
    close(fd);
}

void setup_leds(int isWaiting) {
    int fd;
    char * state = "timer";
    char * delay;
    if(isWaiting) {
        delay = "1000";
    } else {
        delay = "100";
    }
    write_to_sysfs(mmc_led_trigger,   "none");
    write_to_sysfs(usr2_led_trigger,  "none");
    write_to_sysfs(usr3_led_trigger,  "none");
    write_to_sysfs(heartbleed_led_trigger,  state);
    write_to_sysfs(heartbleed_led_delayon,  delay);
    write_to_sysfs(heartbleed_led_delayoff, delay);
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
    pid_t pid; 
    if((pid = fork())<0){
        perror("fork failed");
        exit(1);
    }
    else if(pid!=0){
        printf("forked %d\n",pid);
        FILE * pid_file = fopen(manager_pidfile,"w+");
        fprintf(pid_file,"%d",pid); 
        fclose(pid_file);
        return 0; /* exit parent process */
    }

    setup_leds(WAITING);

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

    sprintf(run_str, "logger %d %s",logger_port, log_dir);


    while (1)
    {
      len = sizeof(cliaddr);
      n = recvfrom(sockfd,mesg, BUF_SIZE ,0 ,(struct sockaddr *)&cliaddr,&len);
      mesg[n] = '\0';
      if(strcmp(mesg,START_CMD) == 0) {
         printf("Starting....\n");
         system(run_str);
         setup_leds(LOGGING);
      }

      if(strcmp(mesg,STOP_CMD) == 0) {
         printf("Stopping....\n");
         int pid = read_pid(logger_pidfile);
         printf("Killing PID %d ....\n",pid);
         if(pid > 0) {
            kill(pid,2);
         }
         setup_leds(WAITING);
      }

      if(strcmp(mesg,SHUTDOWN_CMD) == 0) {
          system("halt -p");
      }
      fflush(NULL);
    }
}
