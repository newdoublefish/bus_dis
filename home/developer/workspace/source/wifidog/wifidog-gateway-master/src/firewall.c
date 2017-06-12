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

/** @internal
  @file firewall.c
  @brief Firewall update functions
  @author Copyright (C) 2004 Philippe April <papril777@yahoo.com>
  2006 Benoit Gr茅goire, Technologies Coeus inc. <bock@step.polymtl.ca>
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <hiredis.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/uio.h>
#include <netdb.h>
#include <sys/time.h>

#include "httpd.h"
#include "safe.h"
#include "util.h"
#include "debug.h"
#include "conf.h"
#include "firewall.h"
#include "fw_iptables.h"
#include "auth.h"
#include "centralserver.h"
#include "client_list.h"
#include "commandline.h"
#include "gateway.h"
#include "ping_thread.h"


static int _fw_deny_raw(const char *, const char *, const int);

/**
 * Allow a client access through the firewall by adding a rule in the firewall to MARK the user's packets with the proper
 * rule by providing his IP and MAC address
 * @param ip IP address to allow
 * @param mac MAC address to allow
 * @param fw_connection_state fw_connection_state Tag
 * @return Return code of the command
 */

void traffic_reload(t_client * client){
	LOCK_CLIENT_LIST();
	client = client_list_find_by_ip(client->ip);
	client->counters.outgoing_history = client->counters.outgoing;
	client->counters.incoming_history = client->counters.incoming;
	UNLOCK_CLIENT_LIST();
}

int fw_fresh(const char *ip)
{
    int result = 0;
	t_client *client;
    int new_fw_connection_state;
	int old_fw_connection_state;

	client = client_dup_by_ip(ip);
	if(client == NULL){
		debug(LOG_ERR, "client already logout\n");
		return -1;
	}
	new_fw_connection_state = client->fw_connection_state;
	old_fw_connection_state = client->old_state;
    debug(LOG_INFO, "Allowing %s with state %d to %d\n", client->ip, old_fw_connection_state,new_fw_connection_state);
	
    /* Grant first */
	//如果不相同，跟新iptables
	if(new_fw_connection_state != old_fw_connection_state){
		debug(LOG_INFO, "state not same fresh iptables\n");
    	result = iptables_fw_access(FW_ACCESS_ALLOW, client->ip, client->mac, new_fw_connection_state);
		if(old_fw_connection_state != -1){
			debug(LOG_INFO, "delete old rules\n");
        	_fw_deny_raw(client->ip, client->mac, old_fw_connection_state);
			traffic_reload(client);
		}else{
			debug(LOG_INFO, "first set rules for %s\n",client->ip);
		}
	}else{
		debug(LOG_INFO, "state same do nothing\n");
	}
	client_free_node(client);
    return result;
}

/**
 * Allow a host through the firewall by adding a rule in the firewall
 * @param host IP address, domain or hostname to allow
 * @return Return code of the command
 */
int
fw_allow_host(const char *host)
{
    debug(LOG_DEBUG, "Allowing %s", host);

    return iptables_fw_access_host(FW_ACCESS_ALLOW, host);
}

/**
 * @brief Deny a client access through the firewall by removing the rule in the firewall that was fw_connection_stateging the user's traffic
 * @param ip IP address to deny
 * @param mac MAC address to deny
 * @param fw_connection_state fw_connection_state Tag
 * @return Return code of the command
 */
int
fw_deny(const char *ip)
{
	t_client *client = client_dup_by_ip(ip);
	if(client == NULL){
		debug(LOG_ERR, "client logout");
		return -1;
	}
    debug(LOG_INFO, "Denying %s:%s with fw_connection_state %d", client->ip, client->mac, client->fw_connection_state);
    _fw_deny_raw(client->ip, client->mac, client->fw_connection_state);
	client_free_node(client);
	return 0;
}

/** @internal
 * Actually does the clearing, so fw_allow can call it to clear previous mark.
 * @param ip IP address to deny
 * @param mac MAC address to deny
 * @param mark fw_connection_state Tag
 * @return Return code of the command
 */
static int
_fw_deny_raw(const char *ip, const char *mac, const int mark)
{
    return iptables_fw_access(FW_ACCESS_DENY, ip, mac, mark);
}

/** Passthrough for clients when auth server is down */
int
fw_set_authdown(void)
{
    debug(LOG_DEBUG, "Marking auth server down");

    return iptables_fw_auth_unreachable(FW_MARK_AUTH_IS_DOWN);
}

/** Remove passthrough for clients when auth server is up */
int
fw_set_authup(void)
{
    debug(LOG_DEBUG, "Marking auth server up again");

    return iptables_fw_auth_reachable();
}

/* XXX DCY */
/**
 * Get an IP's MAC address from the ARP cache.
 * Go through all the entries in config->arp_table_path until we find the
 * requested IP address and return the MAC address bound to it.
 * @todo Make this function portable (using shell scripts?)
 */
char *
arp_get(const char *req_ip)
{
    FILE *proc;
    char ip[16];
    char mac[18];
    char *reply;
    s_config *config = config_get_config();

    if (!(proc = fopen(config->arp_table_path, "r"))) {
        return NULL;
    }

    /* Skip first line */
    while (!feof(proc) && fgetc(proc) != '\n') ;

    /* Find ip, copy mac in reply */
    reply = NULL;
    while (!feof(proc) && (fscanf(proc, " %15[0-9.] %*s %*s %17[A-Fa-f0-9:] %*s %*s", ip, mac) == 2)) {
        if (strcmp(ip, req_ip) == 0) {
            reply = safe_strdup(mac);
            break;
        }
    }

    fclose(proc);

    return reply;
}

/** Initialize the firewall rules
 */
int
fw_init(void)
{
    int result = 0;

    t_client *client = NULL;

    if (!init_icmp_socket()) {
        return 0;
    }

    debug(LOG_INFO, "Initializing Firewall");
    result = iptables_fw_init();

    if (restart_orig_pid) {
        debug(LOG_INFO, "Restoring firewall rules for clients inherited from parent");
        LOCK_CLIENT_LIST();
        client = client_get_first_client();
        while (client) {

            client->fw_connection_state = FW_MARK_NONE;
            fw_fresh(client->ip);
            client = client->next;
        }
        UNLOCK_CLIENT_LIST();
    }

    return result;
}

/** Remove all auth server firewall whitelist rules
 */
void
fw_clear_authservers(void)
{
    debug(LOG_INFO, "Clearing the authservers list");
    iptables_fw_clear_authservers();
}

/** Add the necessary firewall rules to whitelist the authservers
 */
void
fw_set_authservers(void)
{
    debug(LOG_INFO, "Setting the authservers list");
    iptables_fw_set_authservers();
}

/** Remove the firewall rules
 * This is used when we do a clean shutdown of WiFiDog.
 * @return Return code of the fw.destroy script
 */
int
fw_destroy(void)
{
    close_icmp_socket();
    debug(LOG_INFO, "Removing Firewall rules");
    return iptables_fw_destroy();
}

int redisDel_all(void){
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

    reply = redisCommand(c,"flushdb");
    printf("redisDel_all//return: %s\n", reply->str);
    freeReplyObject(reply);

    redisFree(c);

    return 0;
}
int radius_acct (FILE *fp);
int rad_orizen (const char * username,const char *passwd);
int redisSet(char *ip, int value);


/**Probably a misnomer, this function actually refreshes the entire client list's traffic counter, re-authenticates every client with the central server and update's the central servers traffic counters and notifies it if a client has logged-out.
 * @todo Make this function smaller and use sub-fonctions
 */
int safe_acct(const char *ip, int state ){
	FILE *fp_r, *fp_w;
	int ret;
	char buf[256];
	int fd[2];
	t_client *client = client_dup_by_ip(ip);
	if(client == NULL){
		debug(LOG_ERR, "client already logout");
		return -1;
	}
	
	pipe(fd);
	fp_r = fdopen(fd[0],"r");
	if(fp_r == NULL){
		perror("fdopen");
		return -1;
	}
	fp_w = fdopen(fd[1],"w");
	if(fp_w == NULL){
		perror("fdopen");
		return -1;
	}
	if(state != 1){
		//计费或结束
		sprintf(buf,"User-Name=%s\n",client->name);
		fputs(buf,fp_w);

		sprintf(buf,"Acct-Session-Id=\"%s\"\n",client->token);
		fputs(buf,fp_w);

		sprintf(buf,"Acct-Session-Time=%lu\n", time(NULL) - client->counters.login_time);
		fputs(buf,fp_w);

		sprintf(buf,"Acct-Input-Octets=%llu\n",client->counters.incoming);
		fputs(buf,fp_w);

		sprintf(buf,"Acct-Output-Octets=%llu\n",client->counters.outgoing);
		fputs(buf,fp_w);
		
		sprintf(buf,"Acct-Status-Type=%d\n",state);
		fputs(buf,fp_w);
	}else{
		//开始
		sprintf(buf,"User-Name=%s\n",client->name);
		fputs(buf,fp_w);

		sprintf(buf,"Acct-Session-Id=\"%s\"\n",client->token);
		fputs(buf,fp_w);
		
		sprintf(buf,"Acct-Status-Type=%d\n",state);
		fputs(buf,fp_w);
	}
	client_free_node(client);
	fclose(fp_w);
	debug(LOG_INFO, "............................");
	ret = radius_acct(fp_r);
	debug(LOG_INFO, "............................");
	if(ret != 0){
		debug(LOG_ERR, "radius_acct bad\n");
		fclose(fp_r);
		return -1;
	}
	
	fclose(fp_r);
	return 0;
}




void disable_all_auth(void){
    t_client *p1, *worklist;
	int ret;
	
	
    LOCK_CLIENT_LIST();
    ret = client_list_dup(&worklist);
	UNLOCK_CLIENT_LIST();
	debug(LOG_WARNING, "THERE IS %N CLIENT IN LIST!",ret);
	if(ret == 0){
		debug(LOG_WARNING, "NO CLIENT IN LIST!");
		client_list_destroy(worklist);
		return;
	}
	for (p1 = worklist; NULL != p1; p1 = p1->next) {
		//mark 3,redis设置1，表示网络异常
		ret = set_state_access(p1->ip,FW_MARK_JUST_PORTAL,ACCESS_OFFLINE);
		if(ret == -1){
			debug(LOG_ERR, "%s already logout\n",p1->ip);
			continue;
		}
		ret = set_fw_redis_4client(p1->ip,CMD_OLD);
		if(ret == -1){
			debug(LOG_ERR, "%s already logout\n",p1->ip);
			continue;
		}
	}

	client_list_destroy(worklist);
}


void
fw_sync_with_authserver(void)
{
	int rad_ret;
	int set_ret;
	int ret;
    t_client *p1, *worklist;

    LOCK_CLIENT_LIST();
    ret = client_list_dup(&worklist);
	UNLOCK_CLIENT_LIST();
	if(ret == 0){
		debug(LOG_WARNING, "NO CLIENT IN LIST!");
		return;
	}
	for (p1 = worklist; NULL != p1; p1 = p1->next) {

		//检查授权情况
		if(get_pong() != 0){
			disable_all_auth();
			client_list_destroy(worklist);
			return;
		}
		rad_ret = rad_orizen(p1->name,p1->passwd);
		printf("fw_sync_with_authserver 396//after rad_orizen\n");
		if(rad_ret == 1){
			//send 1 :ok!
			set_ret = set_state_access(p1->ip,FW_MARK_KNOWN,ACCESS_YES);
			if(set_ret == -1){
				debug(LOG_ERR, "%s already logout\n",p1->ip);
				continue;
			}
			
			set_ret = set_fw_redis_4client(p1->ip,CMD_OLD);
			if(set_ret == -1){
				debug(LOG_ERR, "%s already logout\n",p1->ip);
				continue;
			}
		}else if(rad_ret == 0){
			set_ret = set_state_access(p1->ip,FW_MARK_JUST_PORTAL,ACCESS_NO);
			if(set_ret == -1){
				debug(LOG_ERR, "%s already logout\n",p1->ip);
				continue;
			}
			
			set_ret = set_fw_redis_4client(p1->ip,CMD_OLD);
			if(set_ret == -1){
				debug(LOG_ERR, "%s already logout\n",p1->ip);
				continue;
			}
		}

    }
    client_list_destroy(worklist);
}

