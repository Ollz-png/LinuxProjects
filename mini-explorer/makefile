# Makefile for File Manager
# Ollz
# Compiler
CXX = g++
CXXFLAGS = -std=c++17 -Wall -g

# Sources
SRC_DIR = src
SRC = $(SRC_DIR)/main.cpp

# Output directory
BIN_DIR = bin
TARGET = $(BIN_DIR)/file-manager

# Libraries via pkg-config
LIBS = $(shell pkg-config --cflags --libs gtk+-3.0)

# Default target
all: $(TARGET)

# Build the program
$(TARGET): $(SRC)
	mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET) $(LIBS)

# Clean build
clean:
	rm -rf $(BIN_DIR)

# Install to /usr/local/bin
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/file-manager
	sudo chmod +x /usr/local/bin/file-manager

.PHONY: all clean install
