/*
 * $Id: radacct.c,v 1.6 2007/07/11 17:29:30 cparker Exp $
 *
 * Copyright (C) 1995,1996 Lars Fenneberg
 *
 * See the file COPYRIGHT for the respective terms and conditions.
 * If the file is missing contact me at lf@elemental.net
 * and I'll send you a copy.
 *
 */

#include <config.h>
#include <includes.h>
#include <freeradius-client.h>
#include <messages.h>
#include <pathnames.h>
#if 1
int radius_acct (FILE *fp);

int main(int argc, char **argv){
	FILE *fp_r, *fp_w;
	int ret;
	char buf[256];
	int fd[2];
	int state;
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

	state = atoi(argv[1]);
	
	if(state == 3){
		sprintf(buf,"User-Name=%s\n","test");
		fputs(buf,fp_w);
		//
		sprintf(buf,"Acct-Session-Id=\"%d\"\n", 9527);
		fputs(buf,fp_w);
		//yes
		sprintf(buf,"Acct-Input-Octets=%d\n", 2);
		fputs(buf,fp_w);
		//yes
		sprintf(buf,"Acct-Output-Octets=%llu\n",2);
		fputs(buf,fp_w);
		//yes
		sprintf(buf,"Acct-Session-Time=%d\n", 2);
		fputs(buf,fp_w);
		//
		sprintf(buf,"Acct-Status-Type=%d\n",3);
		fputs(buf,fp_w);
		
	}else if(state == 2){
		sprintf(buf,"User-Name=%s\n","test");
		fputs(buf,fp_w);
		//
		sprintf(buf,"Acct-Session-Id=\"%d\"\n", 9527);
		fputs(buf,fp_w);
		//yes
		sprintf(buf,"Acct-Input-Octets=%d\n", 3);
		fputs(buf,fp_w);
		//yes
		sprintf(buf,"Acct-Output-Octets=%llu\n",3);
		fputs(buf,fp_w);
		//yes
		sprintf(buf,"Acct-Session-Time=%d\n", 3);
		fputs(buf,fp_w);
		//
		sprintf(buf,"Acct-Status-Type=%d\n",2);
		fputs(buf,fp_w);
	}else if(state == 1){
		sprintf(buf,"User-Name=%s\n","test");
		fputs(buf,fp_w);
		//
		sprintf(buf,"Acct-Session-Id=\"%d\"\n", 9527);
		fputs(buf,fp_w);
		//
		sprintf(buf,"Acct-Status-Type=%d\n",1);
		fputs(buf,fp_w);
	}

	
	fclose(fp_w);
	printf("fclose(fp_w)\n");
	
	ret = radius_acct(fp_r);
	if(ret != 0){
		printf("radius_acct error\n");
		fclose(fp_r);
		printf("fclose(fp_r)\n");
		return -1;
	}
	
	fclose(fp_r);
	printf("fclose(fp_r)\n");
	return 0;
}
#endif
int radius_acct (FILE *fp)
{
	int			result = ERROR_RC;
	VALUE_PAIR	*send = NULL;
   	int			c;
	VALUE_PAIR	*vp;
	DICT_VALUE  *dval;
	char *username, *service, *fproto, *type;
	char *path_radiusclient_conf = RC_CONFIG_FILE;
	rc_handle *rh;


	if ((rh = rc_read_config(path_radiusclient_conf)) == NULL)
		exit(ERROR_RC);

	if (rc_read_dictionary(rh, rc_conf_str(rh, "dictionary")) != 0)
		exit (ERROR_RC);

	if (rc_read_mapfile(rh, rc_conf_str(rh, "mapfile")) != 0)
		exit (ERROR_RC);

	

	if ((send = rc_avpair_readin(rh, fp))) {

		username = service = type = "(unknown)";
		fproto = NULL;

		if ((vp = rc_avpair_get(send, PW_ACCT_STATUS_TYPE, 0)) != NULL)
				if ((dval = rc_dict_getval(rh, vp->lvalue, vp->name)) != NULL) {
					type = dval->name;
					printf("type %s\n",dval->name);
				}

		if ((vp = rc_avpair_get(send, PW_USER_NAME, 0)) != NULL)
				username = vp->strvalue;
			printf("username %s\n",vp->strvalue);

		if ((vp = rc_avpair_get(send, PW_SERVICE_TYPE, 0)) != NULL)
				if ((dval = rc_dict_getval(rh, vp->lvalue, vp->name)) != NULL) {
					service = dval->name;
					printf("service %s\n",dval->name);
				}

		if (vp && (vp->lvalue == PW_FRAMED) &&
			((vp = rc_avpair_get(send, PW_FRAMED_PROTOCOL, 0)) != NULL))
				if ((dval = rc_dict_getval(rh, vp->lvalue, vp->name)) != NULL) {
					fproto = dval->name;
					printf("fproto %s\n",dval->name);
				}

		result = rc_acct(rh, 0, send);
		if (result == OK_RC)
		{
			fprintf(stderr, SC_ACCT_OK);
			rc_log(LOG_NOTICE, "accounting OK, type %s, username %s, service %s%s%s",
				   type, username, service,(fproto)?"/":"", (fproto)?fproto:"");
		}
		else
		{
			fprintf(stderr, SC_ACCT_FAILED, result);
			rc_log(LOG_NOTICE, "accounting FAILED, type %s, username %s, service %s%s%s",
				   type, username, service,(fproto)?"/":"", (fproto)?fproto:"");
		}
		rc_avpair_free(send);
	}

	return result;
}
