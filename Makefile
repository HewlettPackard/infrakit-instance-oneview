TARGET = infrakit-instance-oneview
LIBS = -ljansson -lcurl
LIBPATH = -L./lib/
CC = gcc
CFLAGS = -std=gnu99 -Wall -o3 -s

SRC = src/oneviewHTTP.c \
      src/oneviewHTTPD.c \
      src/oneviewUtils.c \
      src/oneviewQuery.c \
      src/oneviewJSONParse.c \
      src/oneviewInfraKitPlugin.c \
      src/oneviewInfraKitInstance.c \
      src/oneviewInfraKitState.c \
      src/oneviewInfraKitConsole.c \
      infrakit-instance-oneview.c

HEADERS= -I./headers/


.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst ./src/%.c, %.o, $(./src/wildcard *.c))

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(HEADERS) $(SRC) $(CFLAGS) $(LIBPATH) $(LIBS) -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)
