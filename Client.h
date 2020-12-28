#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <malloc.h>
#include <stdlib.h>
#include <netdb.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <time.h>
/* TIMEOUT SET 3 SECOND */
#define WAIT_TIMEOUT 3*1000*1000
/* TFTP MODE */
#define MODE_NETASCII "netascii"
#define MODE_OCTET "octet"
#define MODE_MAIL "mail"
/* upload or download */
#define TYPE_READ 0  
#define TYPE_WRITE 1
/*Operation Code 1-RRQ 2-WRQ 3-DATA 4-ACK 5-ERROR */
#define OPCODE_RRQ 1
#define OPCODE_WRQ 2
#define OPCODE_DATA 3
#define OPCODE_ACK 4
#define OPCODE_ERROR 5

#define DATA_SIZE 512

struct tftp_c { /* tftp connect handle */
    char *mode; /* netascii,octet,mail */
    char *file_name; /* file name */
    int sockfd; /* socketfd to server */
    int type; /* read=0 or write=1 */
    sockaddr_in addr_server; /* server address */
    socklen_t addr_len; /* addrlen 4 socket use */
};


#define TFTP_RRQ_LEN(f,m) (sizeof(tftp_rrq)+strlen(f)+strlen(m)+2)  //calc length of rrq packet
struct tftp_rrq {
    uint16_t opcode;
    char req[];
};
#define TFTP_WRQ_LEN(f,m) (sizeof(tftp_rrq)+strlen(f)+strlen(m)+2)  //calc length of wrq packet
struct tftp_wrq {
    uint16_t opcode;
    char req[];
};

/* recv from server:DATA or ERROR*/
struct tftp_recv_pack
{
    uint16_t opcode;
    uint16_t bnum_ecode; //DATA:BlockNum   ERROR:ErrorCode
    char data[DATA_SIZE];
};

/* packet Data */
struct tftp_data {
    uint16_t opcode;
    uint16_t blocknum;
    char data[DATA_SIZE];
};
/* packet ACK */
struct tftp_ack {
    uint16_t opcode;
    uint16_t blocknum;
};

/* TIMER */
int timer;

/* log file: log.txt */
FILE *log_fp;

/* get time & print */
time_t n_time;
tm* l_time;

/* time for download or upload */
clock_t start_c, end_c;
double time_all;

/* all data size for 1 download or upload */
double size_all;

/* Func Declaration */
tftp_c *tftp_connect(char *host_name, char *port4addr,char *mode, int type, char *file_name);//create connect with server
int tftp_recv(tftp_c *tc);//get file from server
int tftp_put(tftp_c *tc);//put file 2 server
void now_time(void);//get & print now time


