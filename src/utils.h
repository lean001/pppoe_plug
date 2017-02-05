#ifndef PPP_UTILS_H
#define PPP_UTILS_H


int str2int(char* _s, int s_len, unsigned int* _r);
int get_status(char *newip);
char *safe_strdup(const char *s);

#endif