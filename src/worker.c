/*worker.c*/
#include "pppoe_plus.h"
#include "utils.h"
#include "worker.h"
#include "message.h"
#include "conf.h"
#include "debug.h"
#include <pthread.h>
#include <semaphore.h>

extern pthread_mutex_t g_mutex; 
extern sem_t g_sem_full;  
extern sem_t g_sem_empty;
extern char* g_buffer; // maybe problem
extern s_config config;
#ifdef LICENSE
extern long check_counter;
extern long report_counter;
#endif

void* reporter_cb(void* arg){
	char data[MAX_DATA_LEN];
	int rev;
	
	debug(LOG_INFO, "[INFO] reporter begin worker\n");
	while(1){
#ifdef LICENSE
		if(license_limit() || report_counter >= WORKDONE){
			debug(LOG_CRIT, "please contact author: lean");
			sleep(5);
			continue;
		}
		report_counter++;
#endif
		memset(data, 0, MAX_DATA_LEN); 
		
		sem_wait(&g_sem_empty);  
        pthread_mutex_lock(&g_mutex); 
		
		strncpy(data, g_buffer, MAX_DATA_LEN);
		memset(g_buffer, 0, MAX_DATA_LEN);
		
		pthread_mutex_unlock(&g_mutex);
		sem_post(&g_sem_full);  
		
		rev = send_msg(config.report_server.host, config.report_server.port, data);
		if(rev != 0){
			debug(LOG_ERR, "[Err] reporter failed send\n");
		}
		sleep(config.report_time);
	}
	return NULL;
}

void* checker_cb(void* arg){
	char data[MAX_HOST_LEN];
	char *pjson = NULL;
	int rev = -1;
	
	debug(LOG_INFO, "[INFO] Checker checking...\n");
	while(1){
		
#ifdef LICENSE
		if(license_limit() || check_counter >= WORKDONE){
			debug(LOG_CRIT, "please contact author: lean");
			sleep(5);
			continue;
		}
		check_counter++;
#endif
		memset(data, 0, MAX_HOST_LEN);
		
		rev = get_status(data);
		if(rev < 0){
			debug(LOG_ERR, "[Err] checker failed get_status[%d]\n", rev);
			sleep(3);
			continue;
		}
		if(rev == UPDATE)
			pjson = create_msg(rev, data);
		else
			pjson = create_msg(rev, NULL);
		if(!pjson){
			debug(LOG_ERR, "[Err] checker failed create_msg\n");
			sleep(3);
			continue;
		}
		sem_wait(&g_sem_full);
		pthread_mutex_lock(&g_mutex); 
		
		snprintf(g_buffer, MAX_DATA_LEN, "%s", pjson);
		
		debug(LOG_DEBUG,"[DBG] checker: statu %s\n", g_buffer);
		
		pthread_mutex_unlock(&g_mutex); 
		sem_post(&g_sem_empty);
		
		if(pjson){
			free(pjson);
			pjson = NULL;
		}
		sleep(config.check_time);
	}
	return NULL;
}


