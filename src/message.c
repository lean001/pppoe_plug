#include "pppoe_plus.h"
#include "message.h"
#include "debug.h"
#include <curl/curl.h>
#include <time.h>

extern char oldip[MAX_HOST_LEN];
char *msg_json_format = "{\"status\": %d,\"currip\": \"%s\",\"opt\": %d}";
int msg_json_fmt_len = 39;

char* create_msg(int opt, char *content){
	char *data = NULL;
	int len = 0;
	
	switch(opt){
		case OK://ping
			len = OK200_LEN;
			if(!content){
				len += strlen(oldip);
			}else{
				len += strlen(content);
			}
			len += 1; // for opt 0
			break;
			
		case UPDATE:
			len = UPD204_LEN;
			if(!content){
				debug(LOG_ERR, "[Err] the script must be something wrong\n");
				goto error;
			}
			len += strlen(content) + 1;
			sprintf(oldip, "%s",content); 
			break;
			
		case OFFLINE:
			len = OFF408_LEN;
			if(!content){
				len += strlen(oldip);
			}else{
				len += strlen(content);
			}
			len += 1; // for opt 0
			break;	
			
		case SYSERR:
			len = ERR500_LEN;
			if(!content){
				len += strlen(oldip);
			}else{
				len += strlen(content);
			}
			len += 1; // for opt 0
			break;
		
		default:
			debug(LOG_ERR, "create_msg() failed: Unsupport opt[%d] type\n", opt);
			return 0;
	}
	len += msg_json_fmt_len;
	if(len > MAX_DATA_LEN){
		debug(LOG_ERR, "create_msg() failed: data[%d] too long\n", len);
		return 0;
	}
	data = (char *)malloc(len);
	if(!data){
		debug(LOG_ERR, "create_msg() failed: Out of Memory\n");
		return 0;
	}
	memset(data, 0, len);
	switch(opt){
		case OK:
			snprintf(data, len, msg_json_format, OK200, 
				content ? content : oldip, OK);
			break;
		case UPDATE:
			snprintf(data, len, msg_json_format, UPD204, content , UPDATE);
			break;
		case OFFLINE:
			snprintf(data, len, msg_json_format, OFF408, content?content:oldip, OFFLINE);
			break;
		case SYSERR:
			snprintf(data, len, msg_json_format, ERR500, content?content:oldip, SYSERR);
			break;
		default:
			debug(LOG_ERR, "create_msg() failed: Unsupport opt type\n");
			return 0;
	}
	debug(LOG_DEBUG, "create_msg: %s\n", data);
	return data;
error:
	if(data) free(data);
	return 0;
}

/*
* host_url: http://xxxx
* data : json format 
*/
int send_msg(char *host_url, int port, char *data){
	CURL *curl;
	struct curl_slist *plist = NULL;
	CURLcode res;
	long now = time((time_t*)NULL);
	if(!data) return -1;
	
	curl = curl_easy_init();
	if(curl) {
		/* /host?signature=c90588672b2e0c678e27d0962055352ee8d06c32&timestamp=1480083388 */
		res = curl_easy_setopt(curl, CURLOPT_URL, host_url);
		if (res != CURLE_OK){
			debug(LOG_ERR, "curl_easy_setopt(url) failed: %s\n",
				  curl_easy_strerror(res));
			goto error;
		}
		
		res = curl_easy_setopt(curl, CURLOPT_PORT, port);
		if (res != CURLE_OK){
			debug(LOG_ERR, "curl_easy_setopt(port) failed: %s\n",
				  curl_easy_strerror(res));
			goto error;
		}
		plist = curl_slist_append(NULL,"Content-Type:application/json;charset=UTF-8");
		if(!plist){
			debug(LOG_ERR, "curl_slist_append(data) failed\n");
			goto error;
		}
        res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, plist);  
		if (res != CURLE_OK){
			debug(LOG_ERR, "curl_easy_setopt(http head) failed: %s\n",
				  curl_easy_strerror(res));
			goto error;
		}
	#if 0
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, recv_cb);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)chunk);
	#endif
	
		res = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data);
		if (res != CURLE_OK){
			debug(LOG_ERR, "curl_easy_setopt(data) failed: %s\n",
				  curl_easy_strerror(res));
			goto error;
		}

		/* Perform the request, res will get the return code */ 
		res = curl_easy_perform(curl);
		if(res != CURLE_OK){
		  debug(LOG_ERR, "curl_easy_perform() failed: %s\n",
				  curl_easy_strerror(res));
				goto error;
		}
		/* always cleanup */ 
		curl_easy_cleanup(curl);
		return 0;
	}
error:
	if(curl)curl_easy_cleanup(curl);
	return -1;
}