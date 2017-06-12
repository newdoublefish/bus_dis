#include "fcgi_stdio.h" //要写在行首（fcgi_stdio.h里定义的printf与c里的冲突），且用冒号（引用路径而非全局�?
#include <stdio.h>
#include <stdlib.h>
 
int main(void)
{
    int count = 0;
    while(FCGI_Accept() >= 0) {
        printf("Content-type: text/html\r\n");
        printf("\r\n");
        printf("Hello world!<br>\r\n");
        printf("Request number %d.", count++);
    }
    return 0;
}
