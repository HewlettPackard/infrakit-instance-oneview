TARGET = instance-infrakit-oneview
LIBS = -lcurl ljansson
CC = gcc
CFLAGS = -g -Wall

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst ./src/%.c, %.o, $(./src/wildcard *.c))
HEADERS = $(wildcard ./headers/*.h)

%.o: %.c $(HEADERS)
    $(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
    $(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
    -rm -f *.o
    -rm -f $(TARGET)
