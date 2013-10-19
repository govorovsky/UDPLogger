#ifndef LOGGER_H
#define LOGGER_H

 
#define BUF_SIZE 1500+1 // buffer for received UDP, may be different depends on client

enum { label = 0x1337 }; /* for check packets */

typedef struct _Log {
    uint32_t label; /* label to check if packet corrupted */
    int64_t time; /* time of packet receive */
    int32_t pSize; /* size of data in bytes */
    uint32_t cli_addr; /* client addr */
    uint16_t cli_port; /* client port*/
    uint16_t dummy;/* empty for struct aligment */
}__attribute__((packed)) Log; /* prevent system-oriented struct aligment*/




#endif
