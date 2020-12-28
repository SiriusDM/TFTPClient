/* TFTP Client */
#include "Client.h"
using namespace std;

int main(void) {
  char host_name[20];
  char file_name[50];
  char port4addr[20] = "69";
  int type, modetype, flago;
  tftp_c *tc = NULL;
  log_fp = fopen("log.txt", "w+");
  cout << "\nplease enter TFTP-Server host:";
  cin >> host_name;
  cout << "\nplease choose transfer mode (0 => Netascii | 1 => Octet):";
  cin >> modetype;
  while (1) {
    cout << "\nStart or Exit? (0 => Start | 1 => Exit):";
    cin >> flago;
    if (flago) break;
    cout << "\ndownload or upload? (0 => download || 1 => upload):";
    cin >> type;
    cout << "\nplease enter file name:";
    cin >> file_name;
    /* Connect to TFTP Server */
    if (modetype)
      tc = tftp_connect(host_name, port4addr, (char *)MODE_OCTET, type,
                        file_name);
    else
      tc = tftp_connect(host_name, port4addr, (char *)MODE_NETASCII, type,
                        file_name);
    if (!tc) {
      now_time();
      cout << "ERROR:fail to connect to server" << endl;
      fprintf(log_fp, "ERROR:fail to connect to server\n");
      return -1;
    }

    /* Download or Upload */
    if (!type) {  // 0 -> Download
      tftp_recv(tc);
    } else {  // 1 -> Upload
      tftp_put(tc);
    }
  }
  return 0;
}

tftp_c *tftp_connect(char *host_name, char *port4addr, char *mode, int type,
                     char *file_name) {
  tftp_c *tc = NULL;
  addrinfo hints;
  addrinfo *result = NULL;
  tc = (tftp_c *)malloc(sizeof(tftp_c));

  /*
      Create a socket:
      AF_INET:IPV4
      SOCK_DGREAM UDP
      0:auto choose protocal for type
  */
  if ((tc->sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    now_time();
    cout << "ERROR:fail to create socket" << endl;
    fprintf(log_fp, "ERROR:fail to create socket\n");
    free(tc);
    return NULL;
  }

  /* settings 4 getaddrinfo() */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;

  /* get server addr 4 sendto() */
  if (getaddrinfo(host_name, port4addr, &hints, &result) != 0) {
    now_time();
    cout << "ERROR:fail to get server address" << endl;
    fprintf(log_fp, "ERROR:fail to get server address\n");
    free(tc);
    return NULL;
  }
  memcpy(&tc->addr_server, result->ai_addr, result->ai_addrlen);
  tc->addr_len = sizeof(sockaddr_in);
  tc->mode = mode;
  tc->file_name = file_name;
  tc->type = type;
  return tc;
}
/* download from server */
int tftp_recv(tftp_c *tc) {
  tftp_rrq *snd = (tftp_rrq *)malloc(TFTP_RRQ_LEN(tc->file_name, tc->mode));
  tftp_recv_pack recv;
  tftp_ack ack;
  int re, bp_re, stop_flag = 0;
  uint16_t blocknum = 1;
  /* request head */
  snd->opcode = htons(OPCODE_RRQ);
  sprintf(snd->req, "%s%c%s%c", tc->file_name, 0, tc->mode, 0);

  /* send request 2 server */
  re = sendto(tc->sockfd, snd, TFTP_RRQ_LEN(tc->file_name, tc->mode), 0,
              ((sockaddr *)&tc->addr_server), tc->addr_len);
  if (re == -1) {
    now_time();
    cout << "ERROR:fail to send request to server" << endl;
    fprintf(log_fp, "ERROR:fail to send request to server\n");
  }
  now_time();
  cout << "Download Start" << endl;
  fprintf(log_fp, "Download Start");
  FILE *fp = fopen(tc->file_name, "w");
  /* begin recv data */
  start_c = clock();
  size_all = 0;
  while (1) {
    memset(recv.data, 0, sizeof(recv.data));
    for (timer = 0; timer < WAIT_TIMEOUT * 4; timer += 10000) {
      re = recvfrom(tc->sockfd, &recv, sizeof(tftp_recv_pack), MSG_DONTWAIT,
                    ((sockaddr *)&tc->addr_server), &tc->addr_len);
      /* recv last package but not ack */
      if (blocknum == ntohs(recv.bnum_ecode) + 1) {
        ack.opcode = htons(OPCODE_ACK);
        ack.blocknum = recv.bnum_ecode;
        sendto(tc->sockfd, &ack, sizeof(tftp_ack), 0,
               ((sockaddr *)&tc->addr_server), tc->addr_len);
      }
      /* recv success & send ACK */
      if (recv.opcode == htons(OPCODE_DATA) &&
          recv.bnum_ecode == htons(blocknum)) {
        now_time();
        cout << "DATA: BlockNum = " << ntohs(recv.bnum_ecode)
             << ", DataSize = " << (re - 4) << endl;
        fprintf(log_fp,
                "========================DATA================================="
                "\nDATA: BlockNum = %d, DataSize = %d\n",
                ntohs(recv.bnum_ecode), (re - 4));
        // fprintf(fp, "%s", recv.data);
        fwrite(recv.data, 1, re - 4, fp);
        /* record re */
        size_all += re;
        /* Send ACK */
        ack.opcode = htons(OPCODE_ACK);
        ack.blocknum = recv.bnum_ecode;
        sendto(tc->sockfd, &ack, sizeof(tftp_ack), 0,
               ((sockaddr *)&tc->addr_server), tc->addr_len);
        if (re < DATA_SIZE + 4) {
          /* end of recv */
          end_c = clock();
          time_all = (double)(end_c - start_c) / CLOCKS_PER_SEC;
          now_time();
          cout << "Download Finish" << endl;
          fprintf(log_fp, "Download Finsish");

          /* statistic this download */
          fprintf(log_fp, "\n=================================\n");
          // fprintf(log_fp, "file size = %lf kB\n", (size_all) / 1024);
          fprintf(log_fp, "download time = %.2lf s\n", time_all);
          fprintf(log_fp, "this download average throughout  = %.2lf kB/s",
                  size_all / (1024 * time_all));
          fprintf(log_fp, "\n=================================\n");
          stop_flag = 1;
        }
        break;
      }
      usleep(10000);  // recv delay
      now_time();
      /* ========================DEBUG================================= */
      fprintf(
          log_fp,
          "========================Package=================================\n \\
        PackageSize = %d\n \\
        OptionCode = %d\n \\
        BlockNumorErrorCode = %d\n \\
        NextSendBlockNum = %d\n",
          re, ntohs(recv.opcode), ntohs(recv.bnum_ecode), blocknum);
    }
    if (timer >= WAIT_TIMEOUT * 4) {
      if (blocknum == 1) {
        now_time();
        cout << "ERROR:fail to send request to server" << endl;
        fprintf(log_fp, "ERROR:fail to send request to server\n");
        return 0;
      }
      now_time();
      fprintf(log_fp, "ERROR:Data package blocknum = %d timeout\n", blocknum);
      return 0;
    }
    blocknum++;
    if (stop_flag) break;
  }
  fclose(fp);
  return 1;
}

int tftp_put(tftp_c *tc) {
  tftp_wrq *snd = (tftp_wrq *)malloc(TFTP_WRQ_LEN(tc->file_name, tc->mode));
  tftp_recv_pack recv;
  tftp_data snd_data;
  int re = 0, timer, go_flag = 0, stop_flag = 0;
  /* WRQ msg */
  snd->opcode = htons(OPCODE_WRQ);
  FILE *fp = fopen(tc->file_name, "r");
  if (fp == NULL) {
    now_time();
    cout << "ERROR: non-exist file" << endl;
    fprintf(log_fp, "ERROR: non-exist file\n");
    return 0;
  }
  sprintf(snd->req, "%s%c%s%c", tc->file_name, 0, tc->mode, 0);
  /* send WRQ 2 server */
  re = sendto(tc->sockfd, snd, TFTP_WRQ_LEN(tc->file_name, tc->mode), 0,
              ((sockaddr *)&tc->addr_server), tc->addr_len);
  if (re == -1) {
    now_time();
    cout << "ERROR:fail to send request to server" << endl;
    fprintf(log_fp, "ERROR:fail to send request to server");
  }
  /* rec ack timeout*/
  for (timer = 0; timer <= WAIT_TIMEOUT * 4; timer += 10000) {
    re = recvfrom(tc->sockfd, &recv, sizeof(tftp_recv_pack), MSG_DONTWAIT,
                  ((sockaddr *)&tc->addr_server), &tc->addr_len);
    if (recv.opcode == htons(OPCODE_ACK) && recv.bnum_ecode == htons(0)) {
      go_flag = 1;  // ack yes => begin upload
      break;
    }
    usleep(10000);
  }
  if (timer >= WAIT_TIMEOUT * 4) {
    now_time();
    fprintf(log_fp, "ERROR:Recv Server Ack timeout\n");
    return 0;
  }
  if (go_flag) {
    now_time();
    printf("Upload Start\n");
    fprintf(log_fp, "Upload Start\n");
    size_all = 0;
    start_c = clock();
    //    printf("%s\n",tc->file_name);
    int blocknum = 1;
    int size_t;
    snd_data.opcode = htons(OPCODE_DATA);

    while (1) {
      /* init data packet */
      memset(snd_data.data, 0, sizeof(snd_data.data));
      snd_data.blocknum = htons(blocknum);
      size_t = fread(snd_data.data, 1, DATA_SIZE, fp);

      /* send data 2 server */
      for (timer = 0; timer <= WAIT_TIMEOUT; timer += 10000) {
        re = sendto(tc->sockfd, &snd_data, size_t + 4, MSG_DONTWAIT,
                    ((sockaddr *)&tc->addr_server), tc->addr_len);

        /* Count the sum of file */
        size_all += re;

        /* recv from server (ACK|ERROR) */
        re = recvfrom(tc->sockfd, &recv, sizeof(tftp_recv_pack), MSG_DONTWAIT,
                      ((sockaddr *)&tc->addr_server), &tc->addr_len);
        /* ack yes */
        if (recv.opcode == htons(OPCODE_ACK) &&
            recv.bnum_ecode == htons(blocknum)) {
          size_all += re;
          blocknum++;
          now_time();
          cout << "DATA: BlockNum = " << ntohs(recv.bnum_ecode) << endl;
          fprintf(log_fp,
                  "========================DATA================================"
                  "=\nDATA: BlockNum = %d\n",
                  ntohs(recv.bnum_ecode));
          if (size_t != DATA_SIZE) {
            /* end of recv */
            end_c = clock();
            time_all = (double)(end_c - start_c) / CLOCKS_PER_SEC;

            now_time();
            printf("Upload Finish\n");
            fprintf(log_fp, "Upload Finish\n");

            /* statistic this upload */
            fprintf(log_fp, "\n======================================\n");
            //    fprintf(log_fp,"file size = %lf kB\n", (size_all)/1024);
            fprintf(log_fp, "upload time = %.2lf s\n", time_all);
            fprintf(log_fp, "this upload average throughout  = %.2lf kB/s",
                    size_all / (1024 * time_all));
            fprintf(log_fp, "\n======================================\n");
            stop_flag = 1;
          }
          break;
        }
        usleep(10000);
        now_time();
        fprintf(
            log_fp,
            "========================Package=================================\n \\
        PackageSize = %d\n \\ 
        OptionCode = %d\n \\
        BlockNumorErrorCode = %d\n  \\
        NextSendBlockNum = %d\n",
            re, ntohs(recv.opcode), ntohs(recv.bnum_ecode), blocknum);
      }
      if (stop_flag) break;
    }
    fclose(fp);
  }
  return 1;
}

void now_time(void) {
  n_time = time(NULL);
  l_time = localtime(&n_time);
  //   printf("\n%s    ", asctime(l_time));
  fprintf(log_fp, "\n%s    ", asctime(l_time));
  return;
}