#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include "pppoe_plus.h"
#include "debug.h"
#include "utils.h"
#include "worker.h"
#include "conf.h"
#include "httpd.h"

#ifdef LICENSE
	long check_counter;
	long report_counter;
	long update_counter;
#endif


char *OPT="i:f:hp:t:";
char *HELP="Usage:\n"
				"-i : remote host \n"
				"-p : remote server port, default 80\n"
				"-f : config file \n"
				"-c : The time interval of pppoe_plus check the router, default 5s\n"
				"-r : The time interval of send data， default 5 seconds\n"
				"-h : display this help\n"; 

				
s_config config;				
char oldip[MAX_HOST_LEN];
char* cfg_file = 0;
char* g_buffer = NULL;
sem_t g_sem_full;  
sem_t g_sem_empty;  
pthread_mutex_t g_mutex; 

void sig_handle(int signo);
int sig_install();
int init();
void init_daemon();
int main_loop();

int main(int argc, char **argv)
{
	int c;
	pid_t pid;
#ifdef LICENSE
	if(license_limit()){
		debug(LOG_CRIT, "please contact author: lean");
		exit(-1);
	}
#endif
	config_init();
	switch(argc){
		case 1:
			config_read(config.configfile);
			break;
			
		default:
			while((c=getopt(argc,argv,OPT))!=-1) {
				if (c == 'h' || (optarg && strcmp(optarg, "-h") == 0)) {
					printf("%s",HELP);
					return 0;
				}
			}
			optind = 1; /*reset getopt*/
			opterr = 0;
			while((c=getopt(argc,argv,OPT))!=-1) {
				if (c == 'f' || (optarg && strcmp(optarg, "-f") == 0)) {
					config.configfile = optarg;
					config_read(config.configfile);
					break;
				}
			}
			
			optind = 1; /*reset getopt*/
			opterr = 0;
			while((c = getopt(argc, argv, OPT)) != -1){
				switch(c){
					case 'i':
						config.report_server.host = safe_strdup(optarg);
						break;
					case 'p':
						sscanf(optarg, "%d", &config.report_server.port);
						if(config.report_server.port <= 0 || config.report_server.port > 65535){
							debug(LOG_ERR, "Invalid port !!!");
							debug(LOG_ERR, "Exiting...");
							return -1;
						}
						break;
					case 'f':
						break;
					case 'r':
						sscanf(optarg, "%d", &config.report_time);
						break;
					case 'c':
						sscanf(optarg, "%d", &config.check_time);
						break;
					case 'h':
					case '?':
					default:
						printf("%s", HELP);
						return -1;
				}
			}
			break;
	}
	
	config_validate();
	config_init_override();
#ifdef LICENSE
	check_counter = 0;
	report_counter = 0;
	update_counter = 0;
#endif

	if(config.daemon){
		//daemon(0, 1);
		init_daemon();
	}
//	if(init() < 0) goto error;
	if(sig_install() < 0) exit(1);
	pid = fork();
	switch(pid){
		case -1:
			debug(LOG_ERR, "init child failed, exit!\n");
			exit(1);
		case 0:
			if(init() < 0) exit(1);
			main_loop();
			exit(0);
		default:
			break;
	}
	httpd();
	
	return 0;
}

int init(){
	memset(oldip, 0, sizeof(oldip));
	g_buffer = malloc(MAX_DATA_LEN);
	if(!g_buffer) return -1;
	memset(g_buffer, 0, MAX_DATA_LEN);
	
	curl_global_init(CURL_GLOBAL_ALL);
	sem_init(&g_sem_full, 0, 1);  
    sem_init(&g_sem_empty, 0, 0);
	pthread_mutex_init(&g_mutex, NULL); 
	
	return 1;
}

int main_loop(){
	pthread_t checkpid, reportpid;
	
	pthread_create(&reportpid, NULL, reporter_cb, NULL);
	pthread_create(&checkpid, NULL, checker_cb, NULL);
	
	pthread_join(reportpid, NULL);
	pthread_join(checkpid, NULL);
	
	sem_destroy(&g_sem_full);  
    sem_destroy(&g_sem_empty);  
    pthread_mutex_destroy(&g_mutex);
	free(g_buffer);
	curl_global_cleanup();
	
	return 0;
}

void init_daemon()
{
    pid_t pid;

    if ((pid = fork()) == -1) {
        printf("Fork error !\n");
        exit(1);
    }else if (pid != 0) {
        exit(0);        
    }else if(pid == 0){ //child
		int nullfd = open("/dev/null", O_RDWR);
		dup2(nullfd, fileno(stdin));
		dup2(nullfd, fileno(stdout));
		dup2(nullfd, fileno(stderr));
		close(nullfd);
	}

    setsid();           
    if ((pid = fork()) == -1) {
        printf("Fork error !\n");
        exit(-1);
    }
    if (pid != 0) {
        exit(0);        
    }     
    umask(0);
}

int sig_install(){
	if(signal(SIGCHLD, sig_handle) < 0){
		debug(LOG_ERR, "Err: sig_install SIGCHLD failed\n");
		return -1;
	}
	return 1;
}

void sig_handle(int signo){
	pid_t pid;
	int stat;
	
	switch(signo){
		case SIGCHLD:
			while((pid=waitpid(-1, &stat, WNOHANG)) > 0){
				debug(LOG_WARN, "child[%d] terminated\n");
			}
			break;
		default:
			break;
	}
	return;
}