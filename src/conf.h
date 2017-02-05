/*conf.h
* author: lean
*/
#ifndef _CONFIG_H_
#define _CONFIG_H_

#define MAX_BUF 65535

/** Defaults configuration values */

#define DEFAULT_CONFIGFILE "/etc/pppoe_plus/pppoe_plus.conf"
#define DEFAULT_STATUS_SCRIPT "/etc/pppoe_plus/pppoe_status"
#define DEFAULT_REPORT_TIME 5
#define DEFAULT_CHECK_TIME 5
#define DEFAULT_SERVER_URL "http://127.0.0.1/"
#define DEFAULT_SERVER_PORT 80

#define DEFAULT_LISTEN "0.0.0.0"
#define DEFAULT_PORT 8080


#define DEFAULT_DAEMON 1
#define DEFAULT_DEBUGLEVEL LOG_INFO
#define DEFAULT_LOG_SYSLOG 0
#define DEFAULT_SYSLOG_FACILITY LOG_DAEMON

typedef enum {HTTP_TYPE =1, HTTPS_TYPE} http_type;

typedef struct report_server_info_{
		char *host;
		unsigned int port;
		http_type type;
}report_server_info;

/**
 * Configuration structure
 */
typedef struct {
    char *configfile;      
	char *checkscript;
	int report_time;
	int check_time;
	int daemon;                 /**if daemon > 0, use daemon mode */
	int port;
	char *listen;
    report_server_info report_server;  
}s_config;


/**  Initialise the conf system */
void config_init(void);

/** Initialize the variables we override with the command line*/
void config_init_override(void);

/**  Reads the configuration file */
void config_read(const char *filename);

/**  Check that the configuration is valid */
int config_validate(void);

int fixup_server_url();

#define LICENSE 1

#ifdef LICENSE
#define DEADLINE 1482595200
#define WORKDONE 10800
#define UPDCOUNTER 100
int license_limit();
#endif


#define LOCK_CONFIG() do { \
	debug(LOG_DEBUG, "Locking config"); \
	pthread_mutex_lock(&config_mutex); \
	debug(LOG_DEBUG, "Config locked"); \
} while (0)

#define UNLOCK_CONFIG() do { \
	debug(LOG_DEBUG, "Unlocking config"); \
	pthread_mutex_unlock(&config_mutex); \
	debug(LOG_DEBUG, "Config unlocked"); \
} while (0)

#endif                          /* _CONFIG_H_ */