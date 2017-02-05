CC:=gcc
CFLAGS += -DHAVE_CONFIG_H -Wall -g -DFANPF_DEBUG 
TARGET = pppoe_plus

DIRS := $(shell find . -maxdepth 3 -type d)
FILES := $(foreach dir,$(DIRS),$(wildcard $(dir)/*.c))

OBJS := $(patsubst %.c,%.o, $(FILES))

.c.o:
		$(CC) -c  $(CFLAGS)   $< -o $@

LIBS := -lpthread -lcurl
$(TARGET):$(OBJS)
		$(CC) -o $@ $(OBJS) $(LIBS)

clean:
	\/bin/rm -f pppoe_plus
	rm -f *.o
