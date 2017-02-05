/* vim: set et sw=4 ts=4 sts=4 : */


#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>

#include <pthread.h>

#include <string.h>
#include <ctype.h>
#include "pppoe_plus.h"
#include "debug.h"
#include "conf.h"

#include "utils.h"

#ifdef LICENSE

#include <time.h>

int license_limit(){
	long now;
	
	now = time((time_t*)NULL);
	if(now >= DEADLINE) return 1;
	
	return 0;
}

#endif

extern s_config config;


/** 
 * A flag.  If set to 1, there are missing or empty mandatory parameters in the config
 */
static int missing_parms;

typedef enum {
    oServerUrl,
    oServerPort,
	oListen,
	oPort,
    oReportTime,
    oCheckTime,
	oDebugLevel,
	oDaemon,
	oBadOption
} OpCodes;

static const struct {
    const char *name;
    OpCodes opcode;
} keywords[] = {
    {"server_url", oServerUrl}, 
	{"server_port", oServerPort}, 
	{"report_time", oReportTime},
	{"listen", oListen},
	{"port", oPort},
	{"check_time", oCheckTime},
	{"DEBUG", oDebugLevel},	
	{"daemon", oDaemon},
	{NULL, oBadOption}};

static void config_notnull(const void *, const char *);
static OpCodes config_parse_token(const char *, const char *, int);

s_config *
config_get_config(void)
{
    return &config;
}

/** Sets the default config parameters and initialises the configuration system */
void
config_init(void)
{
	memset(&config, 0, sizeof(s_config));
    debug(LOG_DEBUG, "Setting default config parameters");
    config.configfile = safe_strdup(DEFAULT_CONFIGFILE);
    config.checkscript = safe_strdup(DEFAULT_STATUS_SCRIPT);
	
	config.report_time = DEFAULT_REPORT_TIME;
	config.check_time = DEFAULT_CHECK_TIME;
	
	config.report_server.host = safe_strdup(DEFAULT_SERVER_URL);
	config.report_server.port = DEFAULT_SERVER_PORT;
	config.report_server.type = HTTP_TYPE;
	
	config.listen = DEFAULT_LISTEN;
	config.port = DEFAULT_PORT;
	
	config.daemon = -1;
	
	debugconf.log_stderr = 1;
    debugconf.debuglevel = DEFAULT_DEBUGLEVEL;
    debugconf.syslog_facility = DEFAULT_SYSLOG_FACILITY;
    debugconf.log_syslog = DEFAULT_LOG_SYSLOG;
}

/**
 * If the command-line didn't provide a config, use the default.
 */
void
config_init_override(void)
{
    if (config.daemon == -1) {
        config.daemon = DEFAULT_DAEMON;
        if (config.daemon > 0) {
            debugconf.log_stderr = 0;
        }
    }
}

/** @internal
Parses a single token from the config file
*/
static OpCodes
config_parse_token(const char *cp, const char *filename, int linenum)
{
    int i;

    for (i = 0; keywords[i].name; i++)
        if (strcasecmp(cp, keywords[i].name) == 0)
            return keywords[i].opcode;

    debug(LOG_ERR, "%s: line %d: Bad configuration option: %s", filename, linenum, cp);
    return oBadOption;
}


/**
Advance to the next word
@param s string to parse, this is the next_word pointer, the value of s
	 when the macro is called is the current word, after the macro
	 completes, s contains the beginning of the NEXT word, so you
	 need to save s to something else before doing TO_NEXT_WORD
@param e should be 0 when calling TO_NEXT_WORD(), it'll be changed to 1
	 if the end of the string is reached.
*/
#define TO_NEXT_WORD(s, e) do { \
	while (*s != '\0' && !isblank(*s)) { \
		s++; \
	} \
	if (*s != '\0') { \
		*s = '\0'; \
		s++; \
		while (isblank(*s)) \
			s++; \
	} else { \
		e = 1; \
	} \
} while (0)

	
/** @internal
Parses a boolean value from the config file
*/
static int
parse_boolean_value(char *line)
{
    if (strcasecmp(line, "yes") == 0) {
        return 1;
    }
    if (strcasecmp(line, "no") == 0) {
        return 0;
    }
    if (strcmp(line, "1") == 0) {
        return 1;
    }
    if (strcmp(line, "0") == 0) {
        return 0;
    }

    return -1;
}
	
	

/**
@param filename Full path of the configuration file to be read 
*/
void
config_read(const char *filename)
{
    FILE *fd;
    char line[MAX_BUF], *s, *p1, *p2, *rawarg = NULL;
    int linenum = 0, opcode, value;
    size_t len;

    debug(LOG_DEBUG, "Reading configuration file '%s'", filename);

    if (!(fd = fopen(filename, "r"))) {
        debug(LOG_ERR, "Could not open configuration file '%s', " "exiting...", filename);
        exit(1);
    }

    while (!feof(fd) && fgets(line, MAX_BUF, fd)) {
        linenum++;
        s = line;

        if (s[strlen(s) - 1] == '\n')
            s[strlen(s) - 1] = '\0';

        if ((p1 = strchr(s, ' '))) {
            p1[0] = '\0';
        } else if ((p1 = strchr(s, '\t'))) {
            p1[0] = '\0';
        }

        if (p1) {
            p1++;

            // Trim leading spaces
            len = strlen(p1);
            while (*p1 && len) {
                if (*p1 == ' ')
                    p1++;
                else
                    break;
                len = strlen(p1);
            }
            rawarg = safe_strdup(p1);
            if ((p2 = strchr(p1, ' '))) {
                p2[0] = '\0';
            } else if ((p2 = strstr(p1, "\r\n"))) {
                p2[0] = '\0';
            } else if ((p2 = strchr(p1, '\n'))) {
                p2[0] = '\0';
            }
        }

        if (p1 && p1[0] != '\0') {
            /* Strip trailing spaces */

            if ((strncmp(s, "#", 1)) != 0) {
                debug(LOG_DEBUG, "Parsing token: %s, " "value: %s", s, p1);
                opcode = config_parse_token(s, filename, linenum);

                switch (opcode) {
                case oServerUrl:
                    config.report_server.host = safe_strdup(p1);
                    break;
				case oServerPort:
					sscanf(p1, "%d", &config.report_server.port);
					break;
				case oReportTime:
					sscanf(p1, "%d", &config.report_time);
					break;
				case oCheckTime:
					sscanf(p1, "%d", &config.check_time);
					break;
					
				case oListen:
					config.listen = safe_strdup(p1);
					break;
				case oPort:
					sscanf(p1, "%d", &config.port);
					break;
					
				case oDebugLevel:
					sscanf(p1, "%d", &debugconf.debuglevel);
					break;
                case oDaemon:
                    if (config.daemon == -1 && ((value = parse_boolean_value(p1)) != -1)) {
                        config.daemon = value;
                        if (config.daemon > 0) {
                            debugconf.log_stderr = 0;
                        } else {
                            debugconf.log_stderr = 1;
                        }
                    }
                    break;
                case oBadOption:
                    /* FALL THROUGH */
                default:
                    debug(LOG_ERR, "Bad option on line %d " "in %s.", linenum, filename);
                    debug(LOG_ERR, "Exiting...");
                    exit(-1);
                    break;
                }
            }
        }
        if (rawarg) {
            free(rawarg);
            rawarg = NULL;
        }
    }
    fclose(fd);
}

/** Verifies if the configuration is complete and valid.  Terminates the program if it isn't */
int config_validate(void){
	FILE* fd;
    config_notnull(config.report_server.host, "server_url");
    config_notnull(config.checkscript, "checkscript");
    if (!(fd = fopen(config.checkscript, "r"))) {
        debug(LOG_ERR, "Could not open checkscript file '%s', " "exiting...", config.checkscript);
        exit(1);
    }else{ 
		fclose(fd);
	}
	if(fixup_server_url() < 0){
			debug(LOG_ERR, "bad server url %s", config.report_server.host);
			exit(1);
	}
	return 1;
}

/** @internal
    Verifies that a required parameter is not a null pointer
*/
static void
config_notnull(const void *parm, const char *parmname)
{
    if (parm == NULL) {
        debug(LOG_ERR, "%s is not set", parmname);
        missing_parms = 1;
    }
}

int fixup_server_url(){
	char tmp[MAX_HOST_LEN];
	char *ports, *car, *last_car, *re_host = NULL;
	struct in_addr inp;
	int port_len;
	
	re_host = config.report_server.host;
	if(!re_host){
		debug(LOG_ERR, "the URL of the report server is NULL !\n");
		return -1;
	}
	
	if(strncmp(re_host, "http://",7) == 0){
		config.report_server.type = HTTP_TYPE;
		car = re_host + 7;
		config.report_server.port = 80;
	}else if(strncmp(re_host, "https://", 8) == 0){
		config.report_server.type = HTTPS_TYPE;
		car = re_host + 8;
		config.report_server.port = 443;
	}else {
		car = strchr(re_host, ':');
		if(!car){
			car = strchr(re_host, '/');
			if(!car){
				strcpy(tmp, re_host);
			}else{
				strncpy(tmp, re_host, car - re_host);
				tmp[car - re_host]='\0';
			}
			if(inet_aton(tmp, &inp) == 0){
				debug(LOG_ERR, "invalid URL for the report server %s\n",re_host);
				return -1;
			}
		}else{
			strncpy(tmp, re_host, car - re_host);
			tmp[car - re_host]='\0';
			if(inet_aton(tmp, &inp) == 0){
				debug(LOG_ERR, "ERR: invalid URL for the report server %s\n",re_host);
				return -1;
			}
		}
		config.report_server.type = HTTP_TYPE;
		config.report_server.port = 80;
		car = re_host;
	}
	car = strchr(car, ':');
	if(car != NULL){
		ports = car+1;

		if((last_car = strchr(car, '/')) == NULL)
			port_len = strlen(car+1);
		else
			port_len = last_car - car-1;
	
		if(str2int(ports, port_len, &config.report_server.port)){
			debug(LOG_ERR, "ERR: invalid port number for the report server %s\n",re_host);
			return -1;
		}
	}
	return 1;
}
