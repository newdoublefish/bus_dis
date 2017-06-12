/* vim: set et sw=4 ts=4 sts=4 : */
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
/** @file auth.c
    @brief Authentication handling thread
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
#include <unistd.h>
#include <syslog.h>
#include <hiredis.h>

#include "httpd.h"
#include "http.h"
#include "safe.h"
#include "conf.h"
#include "debug.h"
#include "auth.h"
#include "centralserver.h"
#include "fw_iptables.h"
#include "firewall.h"
#include "client_list.h"
#include "util.h"
#include "wd_util.h"
#include "ping_thread.h"
#include "gateway.h"
#include "firewall.h"

/** Launches a thread that periodically checks if any of the connections has timed out
@param arg Must contain a pointer to a string containing the IP adress of the client to check to check
@todo Also pass MAC adress? 
@todo This thread loops infinitely, need a watchdog to verify that it is still running?
*/

void
thread_client_timeout_check(const void *arg){

	t_client *p1, *worklist;
    s_config *config = config_get_config();
	int ret;
	time_t current_time;

	pthread_detach(pthread_self());
	debug(LOG_NOTICE, "==========in thread_client_timeout_check==========");
    if (-1 == iptables_fw_counters_update()) {
        debug(LOG_ERR, "Could not get counters from firewall!");
        return;
    }

	LOCK_CLIENT_LIST();
    ret = client_list_dup(&worklist);
	if(ret == 0){
		debug(LOG_WARNING, "NO CLIENT IN LIST!");
		client_list_destroy(worklist);
		UNLOCK_CLIENT_LIST();
		return;
	}
    UNLOCK_CLIENT_LIST();

	for (p1 = worklist; NULL != p1; p1 = p1->next) {

		ret = safe_acct(p1->ip,ACT_UPLOAD);
		if(ret == -1){
			debug(LOG_ERR, "%s already logout\n",p1->ip);
		}
		
		current_time = time(NULL);
		
		//当客户端已经300 config->clienttimeout秒未更新数据，认为离开，则将做下线处理。(config->checkinterval * 1)
        if (p1->counters.last_updated + 30 <= current_time) {
            /* Timing out user */
            debug(LOG_WARNING, "%s - Inactive for more than %ld seconds, removing client and denying in firewall",
                  p1->ip, config->checkinterval * config->clienttimeout);
            
				//下线,其中修改向raddius上报下线信息。
				//创建新线程，并在线程内使用sem_wAit(A)互斥
			ret = set_fw_redis_4client(p1->ip,CMD_DEL);
			if(ret == -1){
				debug(LOG_ERR, "%s already logout\n",p1->ip);
			}
        } 
	}

	client_list_destroy(worklist);
}


void
thread_client_authority_check(const void *arg)
{
	pthread_detach(pthread_self());
	if(get_pong() == 0){
		debug(LOG_INFO, "fw_sync_with_authserver");
    	fw_sync_with_authserver();
	}else{
		debug(LOG_INFO, "disable_all_auth");
		//外网无法获取，停止计流量，将所有的用户设置为无法上网(仅可以8080)权限
		disable_all_auth();
	}

}

/**
 * @brief Logout a client and report to auth server.
 *
 * This function assumes it is being called with the client lock held! This
 * function remove the client from the client list and free its memory, so
 * client is no langer valid when this method returns.
 *
 * @param client Points to the client to be logged out
 */
int radius_acct (FILE *fp);

int redisDel_ip(const char *ip){
	redisContext *c;
    redisReply *reply;
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
	c = redisConnectWithTimeout("127.0.0.1", 6379, timeout);
    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        return -1;
    }

    /* Set a key */
	
	reply = redisCommand(c,"DEL access#%s",ip);
	printf("redisDel_ip//del access return: %s\n", reply->str);
	freeReplyObject(reply);
	reply = redisCommand(c,"DEL username#%s",ip);
	printf("redisDel_ip//del username return: %s\n", reply->str);
	freeReplyObject(reply);

    redisFree(c);

    return 0;
}

void
logout_client(t_client * client)
{
    client_list_remove(client);
    client_free_node(client);
}