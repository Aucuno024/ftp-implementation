.PHONY: all clean fclean make_dir
.SUFFIXES:
CC=gcc
SRCDIR=src
OBJDIR=obj
EXEC=client serveur_ftp
EXECDIR=bin
SRCS=$(wildcard $(SRCDIR)/*.c)
OBJS = $(SRCS:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
CFLAGS=-Wall -g
CPPFLAGS=-Iinclude
LIBS+=

all: make_dir $(addprefix $(EXECDIR)/,$(EXEC))

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@

# per-target object lists (take care: each exe has its own main)
CLIENT_OBJS := $(OBJDIR)/client.o $(OBJDIR)/csapp.o $(OBJDIR)/request.o $(OBJDIR)/utils.o $(OBJDIR)/response.o
SERVER_OBJS := $(OBJDIR)/serveur_ftp.o $(OBJDIR)/csapp.o $(OBJDIR)/request.o $(OBJDIR)/utils.o $(OBJDIR)/response.o

$(EXECDIR)/client: $(CLIENT_OBJS)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)

$(EXECDIR)/serveur_ftp: $(SERVER_OBJS)
	$(CC) -o $@ $(LDFLAGS) $^ $(LIBS)

make_dir:
	-mkdir -p $(OBJDIR)
	-mkdir -p $(EXECDIR)

clean:
	-rm -rf $(OBJDIR)

fclean:
	-make clean
	-rm -rf $(EXECDIR)
