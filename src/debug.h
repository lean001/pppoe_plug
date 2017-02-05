/* debug.h */

#ifndef _DEBUG_H_
#define _DEBUG_H_

#include <syslog.h>
/*
#define LOG_ERR 1
#define LOG_CRIT LOG_ERR
#define LOG_INFO 2
#define LOG_DEBUG 3
*/
typedef struct _debug_conf {
    int debuglevel;      /**< Debug information verbosity */
    int log_stderr;      /**< Output log to stdout */
    int log_syslog;      /**< Output log to syslog */
    int syslog_facility; /**< facility to use when using syslog for logging */
} debugconf_t;

extern debugconf_t debugconf;

/** Used to output messages.
 * The messages will include the filename and line number, and will be sent to syslog if so configured in the config file 
 * @param level Debug level
 * @param format... sprintf like format string
 */
#define debug(level, format...) _debug(__FILE__, __LINE__, level, format)

/** @internal */
void _debug(const char *, int, int, const char *, ...);

#endif /* _DEBUG_H_ */