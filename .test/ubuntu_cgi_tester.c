// #include <stdio.h>

// int main(void)
// {
//     printf("Status: 200 OK\r\n");
//     printf("Content-Type: text/plain\r\n");
//     printf("Content-Length: 12\r\n");
//     printf("\r\n");
//     printf("Hello World\n");
//     return 0;
// }

#include <stdio.h>

int main(void) {
	int c;
	long count = 0;

	// Read all data from stdin until EOF
	while ((c = getchar()) != EOF) {
		count++;
	}

	// Prepare response body
	char body[64];
	int body_len = snprintf(body, sizeof(body), "%ld", count);

	// CGI response
	printf("Status: 200 OK\r\n");
	printf("Content-Type: text/plain\r\n");
	printf("Content-Length: %d\r\n", body_len);
	printf("\r\n");
	printf("%s", body);

	return 0;
}
