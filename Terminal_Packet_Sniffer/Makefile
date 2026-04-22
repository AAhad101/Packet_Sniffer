# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -I/usr/include/pcap

# Directories
BUILD_DIR = build

# Target executable
TARGET = cshark

# Source files (adjust as needed)
SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

# Default target: build executable
all: $(BUILD_DIR) $(TARGET)

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -lpcap -o $(TARGET)

# Compile source files to object files in build directory
$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build files and executable
clean:
	rm -rf $(BUILD_DIR) $(TARGET)

.PHONY: all clean
