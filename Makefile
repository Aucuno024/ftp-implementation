.PHONY: all clean fclean make_dir test re
.SUFFIXES:
CC=gcc
SRCDIR=src
OBJDIR=obj

EXEC=client serveur_ftp master_dns test
EXECDIR=bin
SRCS=$(wildcard $(SRCDIR)/*.c)
OBJS = $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
CFLAGS=-Wall -g
CPPFLAGS=-Iinclude
LIBS+=-lcrypt

TEST_SRCDIR=src_test
TEST_INCDIR=include_test
TEST_SRCS=$(wildcard $(TEST_SRCDIR)/*.c)
TEST_OBJS=$(TEST_SRCS:$(TEST_SRCDIR)/%.c=$(OBJDIR)/%.o)

ifdef DEBUG
CPPFLAGS+= -DDEBUG -DSLAVE_PORT=1212
endif
ifdef DEFAULT_SERVER_DIR
CPPFLAGS += -DDEFAULT_SERVER_DIR=$(DEFAULT_SERVER_DIR)
else
DEFAULT_SERVER_DIR=serverdir
endif
ifdef DEFAULT_CLIENT_DIR
CPPFLAGS += -DDEFAULT_CLIENT_DIR=$(DEFAULT_CLIENT_DIR)
else
DEFAULT_CLIENT_DIR=clientdir
endif
ifdef PASSWORD
CPPFLAGS += -DPASSWORD=$(PASSWORD)
endif
ifdef SLAVE_PORT
CPPFLAGS+= -DSLAVE_PORT=$(SLAVE_PORT)
endif

ifdef DELAY
$(shell touch $(SRCDIR)/response.c)
CPPFLAGS+= -DDELAY=$(DELAY)
endif


all: make_dir $(addprefix $(EXECDIR)/,$(EXEC))
re: clean all
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJDIR)/%.o: $(TEST_SRCDIR)/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -I$(TEST_INCDIR) $< -o $@


CLIENT_OBJS := $(OBJDIR)/client.o $(OBJDIR)/csapp.o $(OBJDIR)/request.o $(OBJDIR)/utils.o $(OBJDIR)/response.o $(OBJDIR)/logs.o
SERVER_OBJS := $(OBJDIR)/serveur_ftp.o $(OBJDIR)/csapp.o $(OBJDIR)/request.o $(OBJDIR)/utils.o $(OBJDIR)/response.o $(OBJDIR)/logs.o
MASTER_OBJS := $(OBJDIR)/master_dns.o $(OBJDIR)/csapp.o $(OBJDIR)/request.o $(OBJDIR)/utils.o $(OBJDIR)/response.o $(OBJDIR)/logs.o

$(EXECDIR)/client: $(CLIENT_OBJS)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)

$(EXECDIR)/serveur_ftp: $(SERVER_OBJS)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)

$(EXECDIR)/master_dns: $(MASTER_OBJS)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)

$(EXECDIR)/test: $(TEST_OBJS) $(OBJDIR)/csapp.o $(OBJDIR)/request.o $(OBJDIR)/utils.o $(OBJDIR)/response.o $(OBJDIR)/logs.o
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)

make_dir:
	-mkdir -p $(OBJDIR)
	-mkdir -p $(EXECDIR)
	-mkdir -p $(DEFAULT_CLIENT_DIR)
	-mkdir -p $(DEFAULT_SERVER_DIR)

clean:
	-rm -rf $(OBJDIR)

fclean:
	-make clean
	-rfm -rf $(EXECDIR)
	-rm -rf $(DEFAULT_CLIENT_DIR) $(DEFAULT_SERVER_DIR)