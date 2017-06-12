/* vim: set et sw=4 ts=4 sts=4 : */
/********************************************************************\
* This program is free software; you can redistribute it and/or    *
* modify it under the terms of the GNU General Public License as   *
* published by the Free:Software Foundation; either version 2 of   *
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
   @file gateway.c
   @brief Main loop
   @author Copyright (C) 2004 Philippe April <papril777@yahoo.com>
   @author Copyright (C) 2004 Alexandre Carmel-Veilleux <acv@miniguru.ca>
 */

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <time.h>

/* for strerror() */
#include <string.h>

/* for wait() */
#include <sys/wait.h>

/* for unix socket communication*/
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <hiredis.h>


#include "common.h"
#include "httpd.h"
#include "safe.h"
#include "debug.h"
#include "conf.h"
#include "gateway.h"
#include "firewall.h"
#include "commandline.h"
#include "auth.h"
#include "http.h"
#include "client_list.h"
#include "wdctl_thread.h"
#include "ping_thread.h"
#include "httpd_thread.h"
#include "util.h"
#include "httpd_priv.h"
#include "fw_iptables.h"

//#include "redirect_to_rads.h"

/** XXX Ugly hack
 * We need to remember the thread IDs of threads that simulate wait with pthread_cond_timedwait
 * so we can explicitly kill them in the termination handler
 */
static pthread_t tid_ping = 0;

time_t started_time = 0;

/* The internal web server */
httpd * webserver = NULL;


/* Appends -x, the current PID, and NULL to restartargv
 * see parse_commandline in commandline.c for details
 *
 * Why is restartargv global? Shouldn't it be at most static to commandline.c
 * and this function static there? -Alex @ 8oct2006
 */

void http_response(request * r,char *msg){
								httpdSetResponse(r, "200 OK\n");
								_httpd_sendHeaders(r, 0, 0);
								_httpd_sendText(r, msg);
}
int radius_login (const char *user_name, const char *pass_world);

int radius_acct (FILE *fp);
int set_fw_redis_4client(const char *ip,int command){
								int ret;
								switch(command) {
								case CMD_NEW:
																/* Logged in successfully as a regular account */
																debug(LOG_NOTICE, "set new iptables rules for [ip:%s]\n", ip);
																ret = fw_fresh(ip);
																if(ret < 0) {
																								return -1;
																}
																//served_this_session++;
																debug(LOG_NOTICE, "set redis[ip:%s]\n", ip);
																ret = redisSetAccess(ip);
																if(ret == -1) {
																								return -1;
																}
																ret = redisSetUname(ip);
																if(ret == -1) {
																								return -1;
																}
																ret = safe_acct(ip,ACT_START);
																if(ret == -1) {
																								return -1;
																}
																break;
								case CMD_OLD:
																debug(LOG_NOTICE, "fresh iptables rules for [ip:%s]\n", ip);
																ret = fw_fresh(ip);
																if(ret < 0) {
																								return -1;
																}
																debug(LOG_NOTICE, "fresh redis[ip:%s]\n", ip);
																ret = redisSetAccess(ip);
																if(ret == -1) {
																								return -1;
																}
																break;
								case CMD_DEL:
																debug(LOG_NOTICE, "logout [ip:%s]\n", ip);
																ret = fw_deny(ip);
																if(ret == -1) {
																								return -1;
																}
																ret = redisDel_ip(ip);
																if(ret == -1) {
																								return -1;
																}
																ret = safe_acct(ip,ACT_STOP);
																if(ret == -1) {
																								return -1;
																}
																ret = client_list_delete(ip);
																if(ret == -1) {
																								return -1;
																}
																break;
								default: break;
								}

								return 0;
}

int redisSetAccess(const char *ip){
								redisContext *c;
								redisReply *reply;
								int access,old_access;
								int ret;
								struct timeval timeout = { 1, 500000 }; // 1.5 seconds

								c = redisConnectWithTimeout("127.0.0.1", 6379, timeout);
								if (c == NULL || c->err) {
																if (c) {
																								debug(LOG_ERR, "Connection error: %s\n", c->errstr);
																								redisFree(c);
																} else {
																								debug(LOG_ERR, "Connection error: can't allocate redis context\n");
																}
																return -1;
								}

								/* Set a key */
								ret = get_2access(ip,&access,&old_access);
								if(ret == -1) {
																return -1;
								}
								if(old_access != access) {
																reply = redisCommand(c,"SET access#%s %d", ip, access);
																debug(LOG_NOTICE, "SET access#%s %d %s\n", ip, access,reply->str);
																freeReplyObject(reply);
								}else{
																debug(LOG_NOTICE, "access#%s %d same not change\n", ip, access);
								}
								redisFree(c);
								return 0;
}

int redisSetUname(const char *ip){
								redisContext *c;
								redisReply *reply;
								char *username;
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
								username = get_username(ip);
								if(username == NULL) {
																redisFree(c);
																return -1;
								}
								reply = redisCommand(c,"SET username#%s %s", ip, username);
								debug(LOG_NOTICE, "SET username#%s %s %s\n", ip, username,reply->str);
								freeReplyObject(reply);
								free(username);
								redisFree(c);

								return 0;
}


int rad_orizen (const char * username,const char *passwd);
//�Ѿ���½��û��Ȩ�޵��û���ˢ�²鿴�Ƿ���Ȩ�ޣ�����������ͨ������û�У����䡣

void
http_callback_disconnect(httpd * webserver, request * r)
{
								/*sem_wait(sem_t *A);*/
								int ret;

								debug(LOG_INFO, "call disconnect to delete fw/access for ip %s\n",r->clientAddr);
								ret = set_fw_redis_4client(r->clientAddr,CMD_DEL);
								if(ret == -1) {
																http_redirect(r,1,1,"client error Redirect to login");
																return;
								}
								http_redirect(r,1,0,"Redirect to login");

								/*sem_post(sem_t *A);*/
								/*����ʱ�������˳�ʱ��������֤disconnect��fresh��ping�Ļ������á�*/
}

void http_callback_fresh(httpd * webserver, request * r){
								/*sem_wait(sem_t *A);*/
								t_client *client;
								int art_buf;
								int ret;

								debug(LOG_INFO, "fresh fw/access for ip %s\n",r->clientAddr);

								if(get_pong() == -1) {
																debug(LOG_NOTICE,"get pong bad,set state %d access %d for ip:%s\n",FW_MARK_JUST_PORTAL,ACCESS_OFFLINE,r->clientAddr);
																ret = set_state_access(r->clientAddr,FW_MARK_JUST_PORTAL,ACCESS_OFFLINE);
																if(ret == -1) {
																								debug(LOG_ERR,"set_state_access\n");
																								http_redirect(r,1,1,"client error Redirect to login");
																								return;
																}
																ret = set_fw_redis_4client(r->clientAddr,CMD_OLD);
																if(ret == -1) {
																								debug(LOG_ERR,"set_fw_redis_4client\n");
																								http_redirect(r,1,1,"client error Redirect to login");
																								return;
																}
																http_redirect(r,0,1,"Redirect to portal");
																return;
								}
								//��ȡ�ڵ㿽������name passwd
								client = client_dup_by_ip(r->clientAddr);
								if(client == NULL) {
																debug(LOG_ERR,"client already loginout\n");
																http_redirect(r,1,1,"client error Redirect to login");
																client_free_node(client);
																return;
								}

								debug(LOG_INFO,"get pong ok and begin to authorize");
								art_buf = rad_orizen(client->name,client->passwd);
								client_free_node(client);
								printf("http_callback_fresh 202//after rad_orizen\n");
								if(art_buf == 1) {
																//send 1 :ok!
																ret = set_state_access(r->clientAddr,FW_MARK_KNOWN,ACCESS_YES);
																if(ret == -1) {
																								debug(LOG_ERR,"set_state_access\n");
																								http_redirect(r,1,1,"client error Redirect to login");
																								return;
																}
																ret = set_fw_redis_4client(r->clientAddr,CMD_OLD);
																if(ret == -1) {
																								debug(LOG_ERR,"set_fw_redis_4client\n");
																								http_redirect(r,1,1,"client error Redirect to login");
																								return;
																}
								}else if(art_buf == 0) {
																ret = set_state_access(r->clientAddr,FW_MARK_JUST_PORTAL,ACCESS_NO);
																if(ret == -1) {
																								debug(LOG_ERR,"set_state_access\n");
																								http_redirect(r,1,1,"client error Redirect to login");
																								return;
																}
																ret = set_fw_redis_4client(r->clientAddr,CMD_OLD);
																if(ret == -1) {
																								debug(LOG_ERR,"set_fw_redis_4client\n");
																								http_redirect(r,1,1,"client error Redirect to login");
																								return;
																}
								}


								http_redirect(r,0,0,"Redirect to portal");
								/*sem_post(sem_t *A);*/
}

//��ȡ�����������û��������룬������֤��������֤���ص�Ȩ�ޣ����з����ж�
void http_callback_first(httpd * webserver, request * r){

								char username[128];
								char passwd[128];
								char token[128];
								httpVar *tmpVar;
								int art_buf;
								char *mac;
								int ret;

//����֤֮ǰ���ͻ�����Ϣδ���������������Դ��ض��������л�ȡ��
								debug(LOG_INFO,"recv request is %s\n",r->request.path);
//username
								tmpVar = httpdGetVariableByName(r, "username");
								if(tmpVar == NULL) {
																debug(LOG_ERR,"there is no arg username\n");
																http_response(r,"no username\n");
																return;
								}
								strcpy(username, tmpVar->value);
								debug(LOG_NOTICE,"new user is %s",username);

//passwd
								tmpVar = httpdGetVariableByName(r, "pass");
								if(tmpVar == NULL) {
																debug(LOG_ERR,"there is no arg pass\n");
																http_response(r,"no pass");
																return;
								}
								strcpy(passwd, tmpVar->value);
								debug(LOG_NOTICE,"new pass is %s\n",passwd);
//token
								tmpVar = httpdGetVariableByName(r, "token");
								if(tmpVar == NULL) {
																debug(LOG_ERR,"there is no arg token\n");
																http_response(r,"no token\n");
																return;
								}
								strcpy(token, tmpVar->value);
								debug(LOG_NOTICE,"new token is %s\n",token);
//mac
								if (!(mac = arp_get(r->clientAddr))) {
																/* We could not get their MAC address */
																http_response(r,"no mac\n");
																return;
								}
								debug(LOG_NOTICE,"new mac is %s\n",mac);
//ip
								debug(LOG_NOTICE,"new ip is %s\n",r->clientAddr);
//�û��ѵ�½
								if(try_client(r->clientAddr,mac) == 0) {
																debug(LOG_WARNING,"client already login\n");
																http_redirect(r,0,0,"Redirect to portal");
																return;
								}
//��������
								if(get_pong() == -1) {
																debug(LOG_ERR,"get pong bad\n");
																http_redirect(r,1,1,"NETWORK ERROR Redirect to LOGIN");
																return;
								}
								debug(LOG_INFO,"get pong ok and begin to authorize\n");
//��������
								debug(LOG_INFO,"add client to list\n");
								client_list_add(r->clientAddr,mac,token,username,passwd);
								free(mac);

//��Ȩ
								art_buf = rad_orizen(username,passwd);
								debug(LOG_NOTICE,"after rad_orizen art_buf is %d\n",art_buf);
								if(art_buf == ART_YES) {
																//ok!
																//�ض�����portal����ʼ�Ʒ�
																ret = set_state_access(r->clientAddr,FW_MARK_KNOWN,ACCESS_YES);
																if(ret == -1) {
																								debug(LOG_ERR,"set_state_access\n");
																								http_redirect(r,1,1,"client error Redirect to login");
																								return;
																}
																ret = set_fw_redis_4client(r->clientAddr,CMD_NEW);
																if(ret == -1) {
																								debug(LOG_ERR,"set_fw_redis_4client\n");
																								http_redirect(r,1,1,"client error Redirect to login");
																								return;
																}
								}
								else if(art_buf == ART_NO) {
																//no authorizen
																//����8080�˿ڷ���,�ض�����portal
																ret = set_state_access(r->clientAddr,FW_MARK_JUST_PORTAL,ACCESS_NO);
																if(ret == -1) {
																								debug(LOG_ERR,"set_state_access\n");
																								http_redirect(r,1,1,"client error Redirect to login");
																								return;
																}
																ret = set_fw_redis_4client(r->clientAddr,CMD_NEW);
																if(ret == -1) {
																								debug(LOG_ERR,"set_fw_redis_4client\n");
																								http_redirect(r,1,1,"client error Redirect to login");
																								return;
																}
								}

								//portal�ض���
								//�ӳ�1s��Ϊ��iptables��Ч
								usleep(500000);
								http_redirect(r,0,0,"Redirect to portal");
								/*�˴�sem_post(sem_t *A); fresh��disconnect��ping���ſ��Ա�������*/
}


void
append_x_restartargv(void)
{
								int i;

								for (i = 0; restartargv[i]; i++) ;

								restartargv[i++] = safe_strdup("-x");
								safe_asprintf(&(restartargv[i++]), "%d", getpid());
}

/* @internal
 * @brief During gateway restart, connects to the parent process via the internal socket
 * Downloads from it the active client list
 */
void
get_clients_from_parent(void)
{
								int sock;
								struct sockaddr_un sa_un;
								s_config *config = NULL;
								char linebuffer[MAX_BUF];
								int len = 0;
								char *running1 = NULL;
								char *running2 = NULL;
								char *token1 = NULL;
								char *token2 = NULL;
								char onechar;
								char *command = NULL;
								char *key = NULL;
								char *value = NULL;
								t_client *client = NULL;

								config = config_get_config();

								debug(LOG_INFO, "Connecting to parent to download clients");

								/* Connect to socket */
								sock = socket(AF_UNIX, SOCK_STREAM, 0);
								/* XXX An attempt to quieten coverity warning about the subsequent connect call:
								 * Coverity says: "sock is apssed to parameter that cannot be negative"
								 * Although connect expects a signed int, coverity probably tells us that it shouldn't
								 * be negative */
								if (sock < 0) {
																debug(LOG_ERR, "Could not open socket (%s) - client list not downloaded", strerror(errno));
																return;
								}
								memset(&sa_un, 0, sizeof(sa_un));
								sa_un.sun_family = AF_UNIX;
								strncpy(sa_un.sun_path, config->internal_sock, (sizeof(sa_un.sun_path) - 1));

								if (connect(sock, (struct sockaddr *)&sa_un, strlen(sa_un.sun_path) + sizeof(sa_un.sun_family))) {
																debug(LOG_ERR, "Failed to connect to parent (%s) - client list not downloaded", strerror(errno));
																close(sock);
																return;
								}

								debug(LOG_INFO, "Connected to parent.  Downloading clients");

								LOCK_CLIENT_LIST();

								command = NULL;
								memset(linebuffer, 0, sizeof(linebuffer));
								len = 0;
								client = NULL;
								/* Get line by line */
								while (read(sock, &onechar, 1) == 1) {
																if (onechar == '\n') {
																								/* End of line */
																								onechar = '\0';
																}
																linebuffer[len++] = onechar;

																if (!onechar) {
																								/* We have a complete entry in linebuffer - parse it */
																								debug(LOG_DEBUG, "Received from parent: [%s]", linebuffer);
																								running1 = linebuffer;
																								while ((token1 = strsep(&running1, "|")) != NULL) {
																																if (!command) {
																																								/* The first token is the command */
																																								command = token1;
																																} else {
																																								/* Token1 has something like "foo=bar" */
																																								running2 = token1;
																																								key = value = NULL;
																																								while ((token2 = strsep(&running2, "=")) != NULL) {
																																																if (!key) {
																																																								key = token2;
																																																} else if (!value) {
																																																								value = token2;
																																																}
																																								}
																																}

																																if (strcmp(command, "CLIENT") == 0) {
																																								/* This line has info about a client in the client list */
																																								if (NULL == client) {
																																																/* Create a new client struct */
																																																client = client_get_new();
																																								}
																																}

																																/* XXX client check to shut up clang... */
																																if (key && value && client) {
																																								if (strcmp(command, "CLIENT") == 0) {
																																																/* Assign the key into the appropriate slot in the connection structure */
																																																if (strcmp(key, "ip") == 0) {
																																																								client->ip = safe_strdup(value);
																																																} else if (strcmp(key, "mac") == 0) {
																																																								client->mac = safe_strdup(value);
																																																} else if (strcmp(key, "token") == 0) {
																																																								client->token = safe_strdup(value);
																																																} else if (strcmp(key, "fw_connection_state") == 0) {
																																																								client->fw_connection_state = atoi(value);
																																																} else if (strcmp(key, "fd") == 0) {
																																																								client->fd = atoi(value);
																																																} else if (strcmp(key, "counters_incoming") == 0) {
																																																								client->counters.incoming_history = (unsigned long long)atoll(value);
																																																								client->counters.incoming = client->counters.incoming_history;
																																																								client->counters.incoming_delta = 0;
																																																} else if (strcmp(key, "counters_outgoing") == 0) {
																																																								client->counters.outgoing_history = (unsigned long long)atoll(value);
																																																								client->counters.outgoing = client->counters.outgoing_history;
																																																								client->counters.outgoing_delta = 0;
																																																} else if (strcmp(key, "counters_last_updated") == 0) {
																																																								client->counters.last_updated = atol(value);
																																																} else {
																																																								debug(LOG_NOTICE, "I don't know how to inherit key [%s] value [%s] from parent", key,
																																																														value);
																																																}
																																								}
																																}
																								}

																								/* End of parsing this command */
																								if (client) {
																																client_list_insert_client(client);
																								}

																								/* Clean up */
																								command = NULL;
																								memset(linebuffer, 0, sizeof(linebuffer));
																								len = 0;
																								client = NULL;
																}
								}

								UNLOCK_CLIENT_LIST();
								debug(LOG_INFO, "Client list downloaded successfully from parent");

								close(sock);
}

/**@internal
 * @brief Handles SIGCHLD signals to avoid zombie processes
 *
 * When a child process exits, it causes a SIGCHLD to be sent to the
 * process. This handler catches it and reaps the child process so it
 * can exit. Otherwise we'd get zombie processes.
 */
void
sigchld_handler(int s)
{
								int status;
								pid_t rc;

								debug(LOG_DEBUG, "Handler for SIGCHLD called. Trying to reap a child");

								rc = waitpid(-1, &status, WNOHANG);

								debug(LOG_DEBUG, "Handler for SIGCHLD reaped child PID %d", rc);
}

/** Exits cleanly after cleaning up the firewall.
 *  Use this function anytime you need to exit after firewall initialization.
 *  @param s Integer that is really a boolean, true means voluntary exit, 0 means error.
 */
void
termination_handler(int s)
{
								static pthread_mutex_t sigterm_mutex = PTHREAD_MUTEX_INITIALIZER;
								pthread_t self = pthread_self();

								debug(LOG_INFO, "Handler for termination caught signal %d", s);

								/* Makes sure we only call fw_destroy() once. */
								if (pthread_mutex_trylock(&sigterm_mutex)) {
																debug(LOG_INFO, "Another thread already began global termination handler. I'm exiting");
																pthread_exit(NULL);
								} else {
																debug(LOG_INFO, "Cleaning up and exiting");
								}

								debug(LOG_INFO, "Flushing firewall rules...");
								fw_destroy();

								/* XXX Hack
								 * Aparently pthread_cond_timedwait under openwrt prevents signals (and therefore
								 * termination handler) from happening so we need to explicitly kill the threads
								 * that use that
								 */
								if (tid_ping && self != tid_ping) {
																debug(LOG_INFO, "Explicitly killing the ping thread");
																pthread_kill(tid_ping, SIGKILL);
								}

								debug(LOG_NOTICE, "Exiting...");
								exit(s == 0 ? 1 : 0);
}

/** @internal
 * Registers all the signal handlers
 */
static void
init_signals(void)
{
								struct sigaction sa;

								debug(LOG_DEBUG, "Initializing signal handlers");

								sa.sa_handler = sigchld_handler;
								sigemptyset(&sa.sa_mask);
								sa.sa_flags = SA_RESTART;
								if (sigaction(SIGCHLD, &sa, NULL) == -1) {
																debug(LOG_ERR, "sigaction(): %s", strerror(errno));
																exit(1);
								}

								/* Trap SIGPIPE */
								/* This is done so that when libhttpd does a socket operation on
								 * a disconnected socket (i.e.: Broken Pipes) we catch the signal
								 * and do nothing. The alternative is to exit. SIGPIPE are harmless
								 * if not desirable.
								 */
								sa.sa_handler = SIG_IGN;
								if (sigaction(SIGPIPE, &sa, NULL) == -1) {
																debug(LOG_ERR, "sigaction(): %s", strerror(errno));
																exit(1);
								}

								sa.sa_handler = termination_handler;
								sigemptyset(&sa.sa_mask);
								sa.sa_flags = SA_RESTART;

								/* Trap SIGTERM */
								if (sigaction(SIGTERM, &sa, NULL) == -1) {
																debug(LOG_ERR, "sigaction(): %s", strerror(errno));
																exit(1);
								}

								/* Trap SIGQUIT */
								if (sigaction(SIGQUIT, &sa, NULL) == -1) {
																debug(LOG_ERR, "sigaction(): %s", strerror(errno));
																exit(1);
								}

								/* Trap SIGINT */
								if (sigaction(SIGINT, &sa, NULL) == -1) {
																debug(LOG_ERR, "sigaction(): %s", strerror(errno));
																exit(1);
								}
}

/**@internal
 * Main execution loop
 */
static void
main_loop(void)
{
								int result;
								pthread_t tid;
								s_config *config = config_get_config();
								request *r;
								void **params;
								pthread_t ali_tid;

								/* 设置启动时间 */
								if (!started_time) {
																debug(LOG_INFO, "Setting started_time");
																started_time = time(NULL);
								} else if (started_time < MINIMUM_STARTED_TIME) {
																debug(LOG_WARNING, "Detected possible clock skew - re-setting started_time");
																started_time = time(NULL);
								}

								/* save the pid file if needed */
								if ((!config) && (!config->pidfile))
																save_pid_file(config->pidfile);

								/* 获取网关IP，失败退出程序 */
								if (!config->gw_address) {
																debug(LOG_DEBUG, "Finding IP address of %s", config->gw_interface);
																if ((config->gw_address = get_iface_ip(config->gw_interface)) == NULL) {
																								debug(LOG_ERR, "Could not get IP address information of %s, exiting...", config->gw_interface);
																								exit(1);
																}
																debug(LOG_DEBUG, "%s = %s", config->gw_interface, config->gw_address);
								}

								 /* 获取网关ID，失败退出程序 */
								if (!config->gw_id) {
																debug(LOG_DEBUG, "Finding MAC address of %s", config->gw_interface);
																if ((config->gw_id = get_iface_mac(config->gw_interface)) == NULL) {
																								debug(LOG_ERR, "Could not get MAC address information of %s, exiting...", config->gw_interface);
																								exit(1);
																}
																debug(LOG_DEBUG, "%s = %s", config->gw_interface, config->gw_id);
								}

								/* 初始化监听网关2060端口的socket */
								debug(LOG_NOTICE, "Creating web server on %s:%d", config->gw_address, config->gw_port);
								if ((webserver = httpdCreate(config->gw_address, config->gw_port)) == NULL) {
																debug(LOG_ERR, "Could not create web server: %s", strerror(errno));
																exit(1);
								}
								register_fd_cleanup_on_fork(webserver->serverSock);

								debug(LOG_DEBUG, "Assigning callbacks to web server");
#if 0
								httpdAddCContent(webserver, "/", "wifidog", 0, NULL, http_callback_wifidog);
								httpdAddCContent(webserver, "/wifidog", "", 0, NULL, http_callback_wifidog);
								httpdAddCContent(webserver, "/wifidog", "about", 0, NULL, http_callback_about);
								httpdAddCContent(webserver, "/wifidog", "status", 0, NULL, http_callback_status);
#endif
								/* 设置关键路径及其回调函数*/
//    httpdAddCContent(webserver, "/wifidog", "auth", 0, NULL, http_callback_auth);
								httpdAddCContent(webserver, "/wifidog", "disconnect", 0, NULL, http_callback_disconnect);
								//�״ε�½���������������м�Ȩ(���û���������)��1:����->portal,0:������:->portal��������redis
								httpdAddCContent(webserver, "/wifidog", "radius", 0, NULL, http_callback_first);
								//��ֵ�ɹ���������ˢ�·���ǽ
								httpdAddCContent(webserver, "/wifidog", "fresh", 0, NULL, http_callback_fresh);
								/* 设置404错误回调函数，在里面实现了重定向至认证服务器 */
								httpdSetErrorFunction(webserver, 404, http_callback_404);

								/* 清除iptables规则 */
								fw_destroy();
								//ɾ��redis��������
								redisDel_all();
								/* 重新设置iptables规则 */
								if (!fw_init()) {
																debug(LOG_ERR, "FATAL: Failed to initialize firewall");
																exit(1);
								}

								result = pthread_create(&ali_tid, NULL, (void *)fw_alipay, NULL);
								if (result != 0) {
																debug(LOG_ERR, "FATAL: Failed to create a new thread (fw_alipay) -exit(1)");
																perror("thread_client_authority_check");
																pthread_kill(ali_tid, SIGKILL);
																exit(1);
								}


#if 0
								/* Start control thread */
								result = pthread_create(&tid, NULL, (void *)thread_wdctl, (void *)safe_strdup(config->wdctl_sock));
								if (result != 0) {
																debug(LOG_ERR, "FATAL: Failed to create a new thread (wdctl) - exiting");
																termination_handler(0);
								}
								pthread_detach(tid);

								/* Start heartbeat thread */
								//fw_set_authup();
#endif
#if 1
								/* 认证服务器心跳检测线程 */
								result = pthread_create(&tid_ping, NULL, (void *)thread_ping, NULL);
								if (result != 0) {
																debug(LOG_ERR, "FATAL: Failed to create a new thread (ping) - exiting");
																termination_handler(0);
								}
#endif
								debug(LOG_NOTICE, "Waiting for connections");
								while (1) {
																/* 监听2060端口等待用户http请求 */
																r = httpdGetConnection(webserver, NULL);

																/* We can't convert this to a switch because there might be
																 * values that are not -1, 0 or 1. */
																if (webserver->lastError == -1) {
																								/* Interrupted system call */
																								if (NULL != r) {
																																httpdEndRequest(r);
																								}
																} else if (webserver->lastError < -1) {
																								/*
																								 * FIXME
																								 * An error occurred - should we abort?
																								 * reboot the device ?
																								 */
																								debug(LOG_ERR, "FATAL: httpdGetConnection returned unexpected value %d, exiting.", webserver->lastError);
																								if (NULL != r) {
																																httpdEndRequest(r);
																								}
																} else if (r != NULL) {
																								/* 用户http请求接收成功 */
																								params = safe_malloc(2 * sizeof(void *));
																								*params = webserver;
																								*(params + 1) = r;
																								/* 开启http请求处理线程 */
																								result = pthread_create(&tid, NULL, (void *)thread_httpd, (void *)params);
																								if (result != 0) {
																																debug(LOG_ERR, "FATAL: Failed to create a new thread (httpd) - exiting");
																																perror("thread_httpd");
																																pthread_kill(tid, SIGKILL);
																																httpdEndRequest(r);
																								}

																} else {
																								/* webserver->lastError should be 2 */
																								/* XXX We failed an ACL.... No handling because
																								 * we don't set any... */
																}
								}

								/* never reached */
}

/** Reads the configuration file and then starts the main loop */
int gw_main(int argc, char **argv)
{
								//获取空的结构体
								s_config *config = config_get_config();
								//设置结构体默认值
								config_init();
								//执行命令行选项
								parse_commandline(argc, argv);
								/* Initialize the config */
								config_read(config->configfile);
								//检查必要参数，如果没有退出程序
								config_validate();
								/* Initializes the linked list of connected clients */
								client_list_init();
								/* Init the signals to catch chld/quit/etc */
								init_signals();
								if (restart_orig_pid) {
																printf("--------------------------9\n");
																/*
																 * We were restarted and our parent is waiting for us to talk to it over the socket
																 */
																get_clients_from_parent();
																/*
																 * At this point the parent will start destroying itself and the firewall. Let it finish it's job before we continue
																 */
																while (kill(restart_orig_pid, 0) != -1) {
																								debug(LOG_INFO, "Waiting for parent PID %d to die before continuing loading", restart_orig_pid);
																								sleep(1);
																}

																debug(LOG_INFO, "Parent PID %d seems to be dead. Continuing loading.");
								}

								if (config->daemon) {
																printf("--------------------------10\n");

																debug(LOG_INFO, "Forking into background");

																switch (safe_fork()) {
																case 0:        /* child */
																								setsid();
																								append_x_restartargv();
																								main_loop();
																								break;

																default:       /* parent */
																								exit(0);
																								break;
																}
								} else {
																printf("--------------------------12\n");
																append_x_restartargv();
																main_loop();
								}

								return (0);             /* never reached */
}
