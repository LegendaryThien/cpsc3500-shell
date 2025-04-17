CC=g++
CFLAGS=-Wall
SOURCES=myshell.cpp
EXECUTABLE=myshell

all: $(EXECUTABLE)

$(EXECUTABLE): $(SOURCES)
	$(CC) $(CFLAGS) $(SOURCES) -o $@

clean:
	rm -rf $(EXECUTABLE)

.PHONY: all clean 