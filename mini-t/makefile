# Makefile for Mini-T Terminal Emulator
# Ollz

# Compiler
CXX = g++
CXXFLAGS = -std=c++17 -Wall -g

# Sources
SRC = main.cpp

# Output directory
BIN_DIR = bin
TARGET = $(BIN_DIR)/mini-t

# Libraries via pkg-config
LIBS = $(shell pkg-config --cflags --libs gtk+-3.0 vte-2.91)

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
	sudo cp $(TARGET) /usr/local/bin/mini-t
	sudo chmod +x /usr/local/bin/mini-t

.PHONY: all clean install
