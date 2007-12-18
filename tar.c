#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define USAGE { puts("tar [xt]"); exit(EXIT_FAILURE); }
#define FORBLOCK(x,b) for(l = x; l > 0 && (n = read(0,(void *)(b),512)) > 0; l -= n)

/* POSIX tar Header Block, from POSIX 1003.1-1990  */
struct Header
{
	/* Header: 512 Byte */
	char name[100];		/* 0-99 */
	char mode[8];		/* 100-107 */
	char uid[8];		/* 108-115 */
	char gid[8];		/* 116-123 */
	char size[12];		/* 124-135 */
	char mtime[12];		/* 136-147 */
	char chksum[8];		/* 148-155 */
	char typeflag;		/* 156-156 */
	char linkname[100];	/* 157-256 */
	char magic[6];		/* 257-262 */
	char version[2];	/* 263-264 */
	char uname[32];		/* 265-296 */
	char gname[32];		/* 297-328 */
	char devmajor[8];	/* 329-336 */
	char devminor[8];	/* 337-344 */
	char prefix[155];	/* 345-499 */
	char padding[12];	/* 500-512 */
};

int main(int argc, char *argv[]) {
	int a, l, n;
	char b[512];
	struct Header h;

	if((argc != 2 || (a = argv[1][0]) == '\0') ||
			argv[1][1] != '\0' || (a != 't' && a != 'x'))
		USAGE;
	while(1) {
		FORBLOCK(sizeof(h),&h);
		if(*h.name == '\0')
			break;
		else if(l != 0) {
			fputs("Garbage!\n",stderr);
			return EXIT_FAILURE;
		}
		if(a == 't') {
			puts(h.name);
			FORBLOCK(strtoull(h.size,0,8),b);
		}
		else {
			FORBLOCK(strtoull(h.size,0,8),b)
				puts("open, write, close, chmod, chown");
		}
	}
	return EXIT_SUCCESS;
}
