#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

// needed for IO things. Attention that this is different from kernel mode
int lcd;
#define SCULL_IOC_MAGIC 'k'
#define SCULL_HELLO _IO(SCULL_IOC_MAGIC, 1)
#define SCULL_FROM_USER _IOR(SCULL_IOC_MAGIC, 2, char *)
#define SCULL_TO_USER _IOW(SCULL_IOC_MAGIC, 3, char *)
#define SCULL_WR_USER _IOWR(SCULL_IOC_MAGIC, 4, char *)


void test ()
{
	int k, i, sum;
	char s[3];

	memset(s, '2', sizeof(s));
	printf("test begin!\n");

	k = write(lcd, s, sizeof(s));
	printf("written = %d\n", k);

	k = ioctl(lcd, SCULL_HELLO);
	printf("result = %d\n", k);
	
	k = ioctl(lcd, SCULL_FROM_USER, "WRITE");
	printf("result = written length: %d\n", k);
	
	char strread[16];
	k = ioctl(lcd, SCULL_TO_USER, strread);
	printf("result = %s\n", strread);
}

int main(int argc, char ** argv)
{
	lcd = open("/dev/one", O_RDWR);
	if(lcd == -1) {
		perror("unable to open lcd");
		exit(EXIT_FAILURE);
	}

	test();
	
	close(lcd);
	return 0;
}
