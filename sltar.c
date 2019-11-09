/* sltar - a simple tar
 * © 2014 Enno Boland <g s01 de>
 *
 * See LICENSE for further informations
 */
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/sysmacros.h>
#include <ftw.h>
#include <grp.h>
#include <pwd.h>

enum Header {
	NAME = 0, MODE = 100, UID = 108, GID = 116, SIZE = 124, MTIME = 136,
	CHK = 148, TYPE = 156, LINK = 157, MAGIC = 257, VERS = 263, UNAME = 265,
	GNAME = 297, MAJ = 329, MIN = 337, END = 512
};

enum Type {
	REG = '0', HARDLINK = '1', SYMLINK = '2', CHARDEV = '3', BLOCKDEV = '4',
	DIRECTORY = '5', FIFO = '6'
};

void chksum(const char b[END], char *chk) {
	unsigned sum = 0, i;
	for(i = 0; i<END; i++)
		sum += (i >= CHK && i < CHK+8) ? ' ' : b[i];
	snprintf(chk, 8, "%.7o", sum);
}

int c_file(const char* path, const struct stat* st, int type) {
	int l;
	char b[END] = { 0 };
	FILE *f = NULL;
	struct passwd *pw = getpwuid(st->st_uid);
	struct group *gr = getgrgid(st->st_gid);

	memset(b+SIZE, '0', 12);
	strcpy(b+MAGIC, "ustar");
	strcpy(b+VERS, "00");
	snprintf(b+NAME, 100, "%s", path);
	snprintf(b+MODE, 8, "%.7o", (unsigned)st->st_mode&0777);
	snprintf(b+UID,  8, "%.7o", (unsigned)st->st_uid);
	snprintf(b+GID,  8, "%.7o", (unsigned)st->st_gid);
	snprintf(b+MTIME,12, "%.11o", (unsigned)st->st_mtime);
	snprintf(b+UNAME, 32, "%s", pw->pw_name);
	snprintf(b+GNAME, 32, "%s", gr->gr_name);	
	mode_t mode = st->st_mode;
	if(S_ISREG(mode)) {
		b[TYPE] = REG;
		snprintf(b+SIZE, 12, "%.11o", (unsigned)st->st_size);
		f = fopen(path, "r");
	} else if(S_ISDIR(mode)) {
		b[TYPE] = DIRECTORY;
	} else if(S_ISLNK(mode)) {
		b[TYPE] = SYMLINK;
		readlink(path, b+LINK, 99);
	} else if(S_ISCHR(mode)) {
		b[TYPE] = CHARDEV;
		snprintf(b+MAJ,  8, "%.7o", (unsigned)major(st->st_dev));
		snprintf(b+MIN,  8, "%.7o", (unsigned)minor(st->st_dev));
	} else if(S_ISBLK(mode)) {
		b[TYPE] = BLOCKDEV;
		snprintf(b+MAJ,  8, "%.7o", (unsigned)major(st->st_dev));
		snprintf(b+MIN,  8, "%.7o", (unsigned)minor(st->st_dev));
	} else if(S_ISFIFO(mode)) {
		b[TYPE] = FIFO;
	}
	chksum(b, b+CHK);
	fwrite(b, END, 1, stdout);
	if(!f) return 0;
	while((l = fread(b, 1, END, f)) > 0) {
		if(l<END)
			memset(b + l, 0, END - l);
		fwrite(b, END, 1, stdout);
	}
	fclose(f);
	return 0;
}

int x(char *fname, int l, char b[END]){
	static char lname[101] = {0}, chk[8] = {0};
	int r = 0;
	FILE *f = NULL;

	memcpy(lname, b+LINK, 100);
	unlink(fname);
	switch(b[TYPE]) {
	case REG:
		r = !(f = fopen(fname, "w")) ||
			chmod(fname,strtoul(b + MODE, 0, 8));
		break;
	case HARDLINK:
		r = link(lname,fname);
		break;
	case SYMLINK:
		r = symlink(lname,fname);
		break;
	case DIRECTORY:
		r = mkdir(fname,(mode_t) strtoull(b + MODE,0,8));
		break;
	case CHARDEV:
	case BLOCKDEV:
		r = mknod(fname, (b[TYPE] == CHARDEV ?
				S_IFCHR : S_IFBLK) | strtoul(b + MODE,0,8),
					makedev(strtoul(b + MAJ,0,8),
						strtoul(b + MIN,0,8)));
		break;
	case FIFO:
		r = mknod(fname, S_IFIFO | strtoul(b + MODE, 0, 8), 0);
		break;
	default:
		fprintf(stderr,"%s: unsupported filetype %c\n", fname, b[TYPE]);
	}
	if(r || (getuid() == 0 && chown(fname, strtoul(b + UID, 0, 8),
					strtoul(b + GID, 0, 8))))
		perror(fname);
	chksum(b, chk);
	if(strncmp(b+CHK, chk, 8))
		fprintf(stderr, "%s: chksum failed\n", fname);

	for(; l > 0; l -= END){
		fread(b, END, 1, stdin);
		if(f) fwrite(b, MIN(l, 512), 1, f);
	}
	if(f) fclose(f);
	return 0;
}

int t(char *fname, int l, char b[END]){
	puts(fname);
	for(; l > 0; l -= END)
		fread(b, END, 1, stdin);
	return 0;
}

int tar(int (*fn)(char*, int, char[END])) {
	int l;
	char b[END], fname[101] = { 0 };

	while(fread(b, END, 1, stdin)){
		if(*b == '\0')
			break;
		memcpy(fname, b, 100);
		l = strtol(b+SIZE, 0, 8);
		fn(fname, l, b);
	}
	return EXIT_SUCCESS;
}

int c(char *p) {
	static struct stat st;

	if(lstat(p, &st))
		perror(p);
	else if(S_ISDIR(st.st_mode))
		return ftw(p, c_file, 1024);
	else
		return c_file(p, &st, 0);
	return 1;
}

void usage() {
	fputs("sltar-" VERSION " - simple tar\n"
			"Usage: sltar [tx]\n"
			"       sltar c PATH ...\n", stderr);
	exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
	if(argc < 2 || strlen(argv[1]) != 1)
		usage();
	switch(argv[1][0]) {
	case 'c':
		if(argc < 3) usage();
		while(argc-- >= 3)
			if(c(argv[argc])) return EXIT_FAILURE;
		return EXIT_SUCCESS;
	case 'x':
		if(argc == 2) return tar(x);
	case 't':
		if(argc == 2) return tar(t);
	default:
		usage();
	}
	return EXIT_FAILURE;
}
