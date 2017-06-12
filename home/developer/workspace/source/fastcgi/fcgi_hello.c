#include "fcgi_stdio.h" //要写在行首（fcgi_stdio.h里定义的printf与c里的冲突），且用冒号（引用路径而非全局）
#include<sqlite3.h>
#include<stdlib.h>
#include<stdio.h>
#include <string.h>
#define DATABASE_NAME "/home/developer/workspace/database/demo.db"

#define JSON_FORMAT "{\"code\":\"1000\",\"data\":{\"powers\":1,\"type\":0,\"movieList\":[%s]},\"msg\":\"ok\"}"
#define JSON_ITEM_FORMAT "{\"imagePath\":\"%s\",\"name\":\"%s\",\"vedioPath\":\"%s\"}"

int getMovieInfo(){
char ret[1024]={0};
char items[256]={0};
sqlite3_os_init();
sqlite3 *db = NULL;
int rc;
char *sql=NULL;
rc = sqlite3_open(DATABASE_NAME, &db);
if (rc) {
  sqlite3_close(db);
  exit(1);
} else {
//printf("open sqlite file %s succeed;", DATABASE_NAME);
}

int nrow=0,ncolumn=0;
char **result;
char *errMsg;
sql="SELECT * FROM movie_table";
sqlite3_get_table(db,sql,&result,&nrow,&ncolumn,&errMsg);
/*for(int i=0;i<(nrow+1)*ncolumn;i++)
{
	printf("result[%d]=%s\n",i,result[i]);
}*/
for(int i=1;i<nrow+1;i++)
{
	//printf("---------------%d\n",i);
	char item[128]={0};
	for(int j=0;j<ncolumn;j++)
	{
		//printf("%s,",result[i*ncolumn+j]);
	}
	sprintf(item,JSON_ITEM_FORMAT,result[i*ncolumn+2],result[i*ncolumn+1],result[i*ncolumn+3]);
	if(i+1<nrow+1)
		sprintf(item,"%s,",item);
	else
		sprintf(item,"%s",item);
	strcat(items,item);
	//printf("%s\n",items);
}
//printf("%s\n",items);
sprintf(ret,JSON_FORMAT,items);
printf("%s\n",ret);
sqlite3_free_table(result);
sqlite3_close(db);
return 1;
}
 
int main(void)
{
    while(FCGI_Accept() >= 0) {
        printf("Content-type: text/html\r\n");
        printf("\r\n");
       /* printf("Hello world!<br>\r\n");
        printf("Request number %d.", count++);*/
		getMovieInfo();
    }
    return 0;
}
