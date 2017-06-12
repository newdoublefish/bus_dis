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
/** @file centralserver.c
  @brief Functions to talk to the central server (auth/send stats/get rules/etc...)
  @author Copyright (C) 2004 Philippe April <papril777@yahoo.com>
 */

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

#include "httpd.h"
#include <sys/select.h>
#include <unistd.h>
#include <fcntl.h>

/* According to earlier standards */
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "safe.h"
#include "util.h"
#include "wd_util.h"
#include "auth.h"
#include "conf.h"
#include "debug.h"
#include "centralserver.h"
#include "firewall.h"
#include "../config.h"

#include "simple_http.h"

/* Helper function called by connect_auth_server() to do the actual work including recursion
 * DO NOT CALL DIRECTLY
 @param level recursion level indicator must be 0 when not called by _connect_auth_server()
 */
int connect_auth_server()
{
    s_config *config = config_get_config();
    t_auth_serv *auth_server = config->auth_servers;
    //t_popular_server *popular_server = NULL;
    struct in_addr *h_addr;
    //int num_servers = 0;
    char *yunhostname = auth_server->authserv_yunhostname;
    //char *ip;
    struct sockaddr_in their_addr;
    int sockfd;
	short port = 0;

	struct timeval timeout;
	fd_set writeset;
	int ret;
	int opts;
	port = htons(atoi(auth_server->authserv_YunPort));// 服务器端口
	h_addr = wd_gethostbyname(yunhostname);
    their_addr.sin_port = port;
    their_addr.sin_family = AF_INET;
    their_addr.sin_addr = *h_addr;
    memset(&(their_addr.sin_zero), '\0', sizeof(their_addr.sin_zero));
    free(h_addr);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        debug(LOG_ERR, "Failed to create a new SOCK_STREAM socket: %s", strerror(errno));
        return (-1);
    }

	opts = fcntl(sockfd, F_GETFL);  
    if (opts < 0)  
    {  
        perror("fcntl(sock, GETFL)");  
		close(sockfd);
        return -1;  
    }  
    opts = opts|O_NONBLOCK;  
    if (fcntl(sockfd, F_SETFL, opts) < 0)  
    {  
        perror("fcntl(sock, SETFL, opts)");  
		close(sockfd);
        return -1;  
    }  

    if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1) {
        /*
         * Failed to connect
         * Mark the server as bad and try the next one
         */
        timeout.tv_sec  = 1;
	    timeout.tv_usec = 0;
	    FD_ZERO(&writeset);
	    FD_SET(sockfd, &writeset);

		ret = select(sockfd+1, NULL, &writeset, NULL, &timeout);
		if(ret == 0){
			//超时
			printf("connect time out\n");
			close(sockfd);
			return -1;
		}else if(ret == -1){
			perror("select");
			close(sockfd);
			return -1;
		}else{
			//成功
			return sockfd;
		} 
        debug(LOG_DEBUG,
              "Failed to connect to auth server %s:%d (%s). Marking it as bad and trying next if possible",
              yunhostname, ntohs(port), strerror(errno));
        
        //mark_auth_server_bad(auth_server);
        //return _connect_auth_server(level); /* Yay recursion! */

    } else {
        /*
         * We have successfully connected
         */
        debug(LOG_DEBUG, "Successfully connected to auth server %s:%d", yunhostname, ntohs(port));
        return sockfd;
    }
}
