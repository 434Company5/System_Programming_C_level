#
# CMPSC311 - LionCloud Device - Assignment #4
# Makefile - makefile to build the augmented system
#


# Make environment
INCLUDES=-I.
CC=gcc
CFLAGS=-I. -c -g -Wall $(INCLUDES)
LINKARGS=-g
LIBS=-L. -lcmpsc311 -L. -lgcrypt -lpthread -lcurl

# Suffix rules
.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS)  -o $@ $<
	
# Files

TARGETS=	lcloud_client \

CLIENT_OBJECT_FILES=	lcloud_sim.o \
						lcloud_filesys.o \
						lcloud_cache.o \
						lcloud_client.o 

# Productions
all : $(TARGETS)

# Check environment dependencies
prebuild:
	./cmpsc311_prebuild

lcloud_client : $(CLIENT_OBJECT_FILES) $(LCLOUDLIB)
	$(CC) $(LINKARGS) $(CLIENT_OBJECT_FILES) -o $@  -llcloudlib $(LIBS)

clean : 
	rm -f $(TARGETS) $(CLIENT_OBJECT_FILES) 
