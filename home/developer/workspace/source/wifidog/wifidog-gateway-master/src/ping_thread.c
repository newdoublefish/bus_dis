/* vim: set sw=4 ts=4 sts=4 et : */
/********************************************************************\
 * This program is free software; you can redistribute it and/or    *
 * modify it under the terms of the GNU General Public License as   *
 * published by the Free Software Foundation; either version 2 of   *
 * the License, or (at your option) any later version.              *
 *                                                                  *
 * This program is distributed in the hope that it will be useful,  *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of   *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the    *
 * GNU General Public License for more details.                     *
 *                                                                  *
 * You should have received a copy of the GNU General Public License*
 * along with this program; if not, contact:                        *
 *                                                                  *
 * Free Software Foundation           Voice:  +1-617-542-5942       *
 * 59 Temple Place - Suite 330        Fax:    +1-617-542-2652       *
 * Boston, MA  02111-1307,  USA       gnu@gnu.org                   *
 *                                                                  *
\********************************************************************/

/* $Id$ */
/** @file ping_thread.c
    @brief Periodically checks in with the central auth server so the auth
    server knows the gateway is still up.  Note that this is NOT how the gateway
    detects that the central server is still up.
    @author Copyright (C) 2004 Alexandre Carmel-Veilleux <acv@miniguru.ca>
*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <syslog.h>
#include <signal.h>
#include <errno.h>

#include "../config.h"
#include "safe.h"
#include "common.h"
#include "conf.h"
#include "debug.h"
#include "ping_thread.h"
#include "util.h"
#include "centralserver.h"
#include "firewall.h"
#include "gateway.h"
#include "simple_http.h"
#define ON 0
#define OFF -1

static int pong;
static int ping(void);

int get_pong(void){
	return pong;
}
/** Launches a thread that periodically checks in with the wifidog auth server to perform heartbeat function.
@param arg NULL
@todo This thread loops infinitely, need a watchdog to verify that it is still running?
*/
void
thread_ping(void *arg)
{
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;
    struct timespec timeout;
	int count_ok = 0;
	int count_err = 0;
	int after_error = OFF;
	int count = 0;
	int ret;
	pthread_t tid_fw_counter = 0;


	pthread_detach(pthread_self());
    while (1) {
        /* Make sure we check the servers at the very begining */
        
        pong = ping();
		debug(LOG_INFO, "==========ping  %s==========",!pong?"ok":"bad");
		if(0 == pong){
			//网络通畅，count_err清零
			count_ok++;
			count++;
			count_err = 0;
			if(after_error != OFF){
				if(++after_error == 5){
					//在三次失败后联网成功5次，进行鉴权
					count_ok = 0;
					debug(LOG_NOTICE,"==========after no net,toke all user to authorize==========");
					ret = pthread_create(&tid_fw_counter, NULL, (void *)thread_client_authority_check, NULL);
				    if (ret != 0) {
				        debug(LOG_ERR, "FATAL: Failed to create a new thread (thread_client_authority_check) - exiting");
				        perror("thread_client_authority_check");
		        		pthread_kill(tid_fw_counter, SIGKILL);
		    		}
					after_error = OFF;
				}
			}
		}else{
			//网络异常
			count_err++;
			if(after_error != OFF){
				after_error = ON;
			}
		}


		if(12*5 == count){
			//网络正常5分钟刷新一次
			count = 0;
			debug(LOG_NOTICE, "==========upload the date for every one==========");
			ret = pthread_create(&tid_fw_counter, NULL, (void *)thread_client_timeout_check, NULL);
		    if (ret != 0) {
		        debug(LOG_ERR, "FATAL: Failed to create a new thread (thread_client_timeout_check) - exiting");
		        perror("thread_client_timeout_check");
        		pthread_kill(tid_fw_counter, SIGKILL);
    		}
		}

		
		if(12*10 == count_ok){
			//网路正常5分钟刷新一次
			count_ok = 0;
			debug(LOG_NOTICE,"nomall,toke all user to authorize\n");
			ret = pthread_create(&tid_fw_counter, NULL, (void *)thread_client_authority_check, NULL);
		    if (ret != 0) {
		        debug(LOG_ERR, "FATAL: Failed to create a new thread (thread_client_authority_check) - exiting");
		        perror("thread_client_authority_check");
        		pthread_kill(tid_fw_counter, SIGKILL);
    		}
		}else if(after_error == OFF){

			if(5 == count_err){
				//连续三次(25s)网络异常，调用thread_client_timeout_check线程，mark所有用户为3
				count_err = 0;
				after_error = ON;
				debug(LOG_NOTICE,"no network avilable,mark all user 3\n");
				ret = pthread_create(&tid_fw_counter, NULL, (void *)thread_client_authority_check, NULL);
			    if (ret != 0) {
			        debug(LOG_ERR, "FATAL: Failed to create a new thread (thread_client_authority_check) - exiting");
			        perror("thread_client_authority_check");
		    		pthread_kill(tid_fw_counter, SIGKILL);
				}
			}
		}
        /* Sleep for config.checkinterval seconds... */
		/*ping radius server every 5s*/
        timeout.tv_sec = time(NULL) + 5;
        timeout.tv_nsec = 0;

        /* Mutex must be locked for pthread_cond_timedwait... */
        pthread_mutex_lock(&cond_mutex);

        /* Thread safe "sleep" */
        pthread_cond_timedwait(&cond, &cond_mutex, &timeout);

        /* No longer needs to be locked */
        pthread_mutex_unlock(&cond_mutex);
    }
}

/** @internal
 * This function does the actual request.
 */
static int ping(void)
{
    char request[MAX_BUF];
    //FILE *fh;
    int sockfd;
    //unsigned long int sys_uptime = 0;
    //unsigned int sys_memfree = 0;
    //float sys_load = 0;
    t_auth_serv *auth_server = NULL;
    auth_server = get_auth_server();

    debug(LOG_WARNING, "Entering ping()");
    memset(request, 0, sizeof(request));
    /*
     * The ping thread does not really try to see if the auth server is actually
     * working. Merely that there is a web server listening at the port. And that
     * is done by connect_auth_server() internally.
     */
    sockfd = connect_auth_server();
    if (sockfd == -1) {
 		printf("ping()/creat sockfd error pong error\n");
        return -1;
    }

    /*
     * Prep & send request
     */
    snprintf(request, sizeof(request) - 1,
             "GET %s%s HTTP/1.0\r\n"
             "User-Agent: DevilYang %s\r\n"
             "Host: %s\r\n"
             "\r\n",
             auth_server->authserv_path,
             auth_server->authserv_ping_script_path_fragment,
             VERSION, 
             auth_server->authserv_yunhostname);

    char *res;

	res = http_get(sockfd, request);
    if (NULL == res) {
        debug(LOG_ERR, "ping thread: pong error *!");
		close(sockfd);
		return -1;
    } else if (strstr(res, "pong") == 0) {
        debug(LOG_WARNING, "ping thread: pong error **!");
		close(sockfd);
		free(res);
		return -1;
    } else {
        debug(LOG_WARNING, "ping thread: pong ok");
		close(sockfd);
        free(res);
		return 0;
    }

}
