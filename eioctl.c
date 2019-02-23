/* 
 * eioctl - simple cmdline tool to setup and control EnhanceIO caching module
 *
 * Written by A$teroid, 2019 
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define VERSION1 0
#define VERSION2 2

#define EIODEV "/dev/eiodev"

#define EIO_IOC_CREATE 1104168192
#define EIO_IOC_DELETE 1104168193
#define EIO_IOC_ENABLE 1104168194
#define EIO_IOC_DISABLE 1104168195


typedef enum
{ false, true } bool;

typedef struct
{
	char name[32];
	char srcname[128];
	char ssdname[128];
	char ssduuid[128];
	uint64_t srcsize;
	uint64_t ssdsize;
	unsigned int src_sector;
	unsigned int ssd_sector;
	unsigned int flags;
	unsigned char policy;
	unsigned char mode;
	unsigned char persistence;
	unsigned long long blksize;
	unsigned long long assoc;
} self;

self cachedev;
int eio_act;

int version();
int help();

int do_ioctl(int ioctl_nr, void *data)
{
	int fd;
	fd = open(EIODEV, O_RDONLY, S_IRUSR);
	if (!ioctl(fd, ioctl_nr, data))
	{
		close(fd);
		return 0;
	}
	else
		return 1;
}

uint64_t dsize(char device[128])
{
	int fd;
	uint64_t numblocks;

	fd = open(device, O_RDONLY);
	ioctl(fd, BLKGETSIZE64, &numblocks);

	close(fd);

	return numblocks;
}

static bool parse_args(int argc, char **argv)
{
	int o;
	int m;
	int t, c, n;

	while ((o = getopt(argc, argv, "t:c:n:hader:m:p:b:v")) != -1)
	{
		switch (o)
		{
		case 'h':
			exit(help());
			break;

		case 'v':
			exit(version());
			break;

		case 'p':
			m = atoi(optarg);
			if (m < 1 || m > 3)
			{
				printf("policy must be 1, 2 or 3\n");
				exit(1);
			}
			else
				cachedev.policy = m;

			break;

		case 'm':
			m = atoi(optarg);
			if (m < 1 || m > 3)
			{
				printf("mode must be 1, 2 or 3\n");
				exit(1);
			}
			else
				cachedev.mode = m;

			break;

		case 'a':
			eio_act = EIO_IOC_CREATE;
			break;

		case 'e':
			eio_act = EIO_IOC_ENABLE;
			break;

		case 'r':
			eio_act = EIO_IOC_DELETE;
			if (optarg != NULL)
		 	sprintf(cachedev.name, "%s", optarg);
		 	else
		 	 exit(1);
			break;

		case 'd':
			eio_act = EIO_IOC_DISABLE;
			break;

		case 'b':
			m = atoi(optarg);
			if (m == 2048 || m == 4096 || m == 8192)
			{
				cachedev.blksize = m;
				cachedev.assoc = (m / 16);
			}
			else
			{
				printf("%s %d\n", "invalid block size", m);
				exit(1);
			}
			break;

		case 'n':
			if (sizeof(optarg) > 32)
				exit(1);
			sprintf(cachedev.name, "%s", optarg);
			n = 1;
			break;

		case 'c':
			if (sizeof(optarg) > 128)
				exit(1);


			sprintf(cachedev.ssdname, "%s", optarg);
			c = 1;
			break;

		case 't':
			if (sizeof(optarg) > 128)
				exit(1);


			sprintf(cachedev.srcname, "%s", optarg);
			t = 1;
			break;

		case '?':
			return false;
			break;
		}
		m = 0;
	}

	if(eio_act != EIO_IOC_DELETE && n == 0)
		exit(help());

	if ((t == 0 || c == 0) && (eio_act == EIO_IOC_CREATE || eio_act == EIO_IOC_ENABLE))
		exit(help());

	return true;
}

void show_config()
{
	printf("Name	=	%s\n", cachedev.name);
	printf("Target	=	%s\n", cachedev.srcname);
	printf("Cache	=	%s\n", cachedev.ssdname);
	printf("Target size	=	%lld\n", cachedev.srcsize);
	printf("Cache size	=	%lld\n", cachedev.ssdsize);
	printf("Block size	=	%lld\n", cachedev.blksize);

	if (cachedev.mode > 1)
	{
		if (cachedev.mode > 2)
			printf("Caching mode	=	writethrout\n");
		else
			printf("Caching mode	=	read only\n");
	}
	else
		printf("Caching mode	=	writeback\n");

	if (cachedev.policy > 1)
	{
		if (cachedev.policy > 2)
			printf("Policy	=	random\n");
		else
			printf("Policy	=	last recent used (LRU)\n");
	}
	else
		printf("Policy	=	FIFO\n");
}

int main(int argc, char **argv)
{
	int ret;

	cachedev.mode = 3;
	cachedev.policy = 2;
	cachedev.assoc = 256;
	cachedev.blksize = 4096;
	cachedev.src_sector = 512;
	cachedev.ssd_sector = 512;
	cachedev.flags = 0;

	if (argc > 1)
	{
		parse_args(argc, argv);
	}
	else
		return help();


	cachedev.srcsize = dsize(cachedev.srcname);
	if (cachedev.srcsize == 0)
	{
		printf("target device is busy or invalid");
		exit(1);
	}

	cachedev.ssdsize = dsize(cachedev.ssdname);
	if (cachedev.ssdsize == 0)
	{
		printf("cache device is busy or invalid\n");
		exit(1);
	}

	if(eio_act != EIO_IOC_DELETE)
		show_config();
	else
		printf("Name	=	%s\n", cachedev.name);

	ret = do_ioctl(eio_act, &cachedev);
	if (ret > 0)
		printf("Failed\n");
	else
		printf("Successful\n");

	return 0;
}

int help()
{
	version();
	printf
		("\nUsage:\n eioctl -a/e/d/r -t [target] -c [cache] -m [mode] -p [policy] -b [blksize] -n [name]\n\n-a     add cache\n-e      enable cache (-d is disable)\n-r $name      remove cache with $name\n\n-t [target]      where [target] is device which will be cached\n-c [cache]      where [cache] is fast device which will be a cache\n-n [name]      is cache name\n-m [mode]      caching mode, can be 1, 2 or 3 for writeback, read-only and writethrout modes\n-p [policy]      is a policy for caching. Can be 1, 2 or 3 for FIFO, LRU and random policies\n-b [blksize]      is block size (valid 2048, 4096 of 8192 values\n\n");
		printf("Default caching mode is writethrout with LRU policy.");
	return 0;
}

int version()
{
	printf("%s%u.%u\n", "eioctl - commandline EnhanceIO control tool.\nVersion ",
		   VERSION1, VERSION2);
	return 0;
}