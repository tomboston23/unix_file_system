# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Source and object files
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)
TARGET = myfs

# Default rule
all: $(TARGET)
	./$(TARGET)

t: 
	make $(TARGET)


run: myfs
	./myfs

# Linking
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

# Compile .c to .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(OBJS) $(TARGET) && rm *.o
