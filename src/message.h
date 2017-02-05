#ifndef MESSAGE_H
#define MESSAGE_H


#define OK200 200
#define OK200s "200"
#define OK200_LEN (sizeof(OK200s) - 1)

#define UPD204 204
#define UPD204s "204"
#define UPD204_LEN (sizeof(UPD204s) - 1)

#define OFF408 408
#define OFF408s "408"
#define OFF408_LEN (sizeof(OFF408s) - 1)

#define ERR500 500
#define ERR500s "500"
#define ERR500_LEN (sizeof(ERR500s) - 1)


char* create_msg(int opt, char *content);
int send_msg(char *host_url, int port, char *data);

#endif