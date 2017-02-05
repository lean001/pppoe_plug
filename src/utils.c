#include <signal.h>
#include <errno.h>
#include "pppoe_plus.h"
#include "utils.h"
#include "conf.h"
#include "debug.h"

extern s_config config;
/*
 * Convert a str into integer
 */
int str2int(char* _s, int s_len, unsigned int* _r) 
{
    int i;
        
    *_r = 0;
    for(i = 0; i < s_len; i++) {
        if ((_s[i] >= '0') && (_s[i] <= '9')) {
            *_r *= 10; 
            *_r += _s[i] - '0';
        } else {
            return -1; 
        }   
    }   
        
    return 0;
}

char *
safe_strdup(const char *s)
{
    char *retval = NULL;
    if (!s) {
        debug(LOG_CRIT, "safe_strdup called with NULL which would have crashed strdup. Bailing out");
        exit(1);
    }
    retval = strdup(s);
    if (!retval) {
        debug(LOG_CRIT, "Failed to duplicate a string: %s.  Bailing out", strerror(errno));
        exit(1);
    }
    return (retval);
}

int get_status(char *newip){
	char buf[127];
	FILE *rev_stream = NULL;
	void *prev;
	char *p;
	int len;
	
	if(!newip){
		return -1;
	}
	prev = signal(SIGCHLD,SIG_DFL);
	rev_stream = popen(config.checkscript, "r");
	if(!rev_stream){
		signal(SIGCHLD,prev);
		return -1;
	}
	p = fgets(buf, 127, rev_stream);
	pclose(rev_stream);
	signal(SIGCHLD,prev);
	
	len = strlen(p);
	if(len > 2){
		snprintf(newip, 127, "%s", p);
		return UPDATE;
	}else{
		switch(*p){
			case '0':
				return OK;
			case '3':
				return OFFLINE;
			case '4':
				return SYSERR;
			default:
				return -1;
		}
	}
	return -1;
}



