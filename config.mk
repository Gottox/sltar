# sltar version
VERSION = 0.2

# Customize below to fit your system

# paths
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# includes and libs
INCS = -I. -I/usr/include
LIBS = -L/usr/lib -lc

# flags
CFLAGS = -static -Os -g -Wall -Werror ${INCS} -DVERSION=\"${VERSION}\"
LDFLAGS = -g ${LIBS}

# compiler and linker
CC = cc
