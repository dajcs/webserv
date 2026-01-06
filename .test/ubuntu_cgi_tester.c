#include <stdio.h>

int main(void)
{
    printf("Status: 200 OK\r\n");
    printf("Content-Type: text/plain\r\n");
    printf("Content-Length: 12\r\n");
    printf("\r\n");
    printf("Hello World\n");
    return 0;
}

