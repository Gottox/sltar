/* sltar - suckless tar
 * Copyright (C) <2007> Enno boland <g s01 de>
 *
 * See LICENSE for further informations
 */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

enum Header {
	MODE = 100, UID = 108, GID = 116, SIZE = 124, MTIME = 136,
	TYPE = 156, LINK = 157, MAJ = 329, MIN = 337, END = 512
};

int main(int argc, char *argv[]) {
	int a, l;
	char b[END],fname[101],lname[101];
	FILE *f = 0;

	if((argc != 2 || (a = argv[1][0]) == '\0') ||
			argv[1][1] != '\0' || (a != 't' && a != 'x')) {
		fputs("sltar-" VERSION " - suckless tar\nsltar [xt]\n",stderr);
		return EXIT_FAILURE;
	}
	for(lname[100] = fname[100] = l = 0; fread(b,END,1,stdin); l -= END)
		 if(l <= 0) {
			if(*b == '\0')
				break;
			memcpy(fname,b,100);
			memcpy(lname,b + LINK,100);
			l = strtoull(b + SIZE,0,8) + END;
			if(a == 't') {
				puts(fname);
				continue;
			}
			if(f) {
				fclose(f);
				f = 0;
			}
			unlink(fname);
			switch(b[TYPE]) {
			case '0': /* file */
				if(!(f = fopen(fname,"w")) || chmod(fname,strtoul(b + MODE,0,8)))
					perror(fname);
				break;
			case '1': /* hardlink */
				if(!link(lname,fname))
					perror(fname);
				break;
			case '2': /* symlink */
				if(!symlink(lname,fname))
					perror(fname);
				break;
			case '5': /* directory */
				if(mkdir(fname,(mode_t) strtoull(b + MODE,0,8)))
					perror(fname);
				break;
			case '3': /* char device */
			case '4': /* block device */
				if(mknod(fname, (b[TYPE] == '3' ? S_IFCHR : S_IFBLK) | strtoul(b + MODE,0,8),
						makedev(strtoul(b + MAJ,0,8),
							strtoul(b + MIN,0,8))))
					perror(fname);
				break;
			case '6': /* fifo */
				if(mknod(fname, S_IFIFO | strtoul(b + MODE,0,8), 0))
					perror(fname);
				break;
			default:
				fprintf(stderr,"not supported filetype %c\n",b[TYPE]);
			}
			if(getuid() == 0 && chown(fname, strtoul(b + UID,0,8),strtoul(b + GID,0,8)))
				perror(fname);
		}
		else if(a == 'x' && f && !fwrite(b,l > 512 ? END : l,1,f)) {
			perror(fname);
			break;
		}
	if(f)
		fclose(f);
	return EXIT_SUCCESS;
}
