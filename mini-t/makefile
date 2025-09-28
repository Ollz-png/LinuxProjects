# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wshadow -g `pkg-config --cflags gtk+-3.0 vte-2.91`
LDFLAGS = `pkg-config --libs gtk+-3.0 vte-2.91`

# Directories
SRC_DIR = src
OBJ_DIR = obj
INCLUDE_DIR = include

# Source files
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
DEP := $(OBJ:.o=.d)   # dependency files

# Target executable
TARGET = mini-t

# Default rule
all: $(TARGET)

# Compile object files with dependency generation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -MMD -MP -c $< -o $@

# Ensure obj directory exists
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Link executable
$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)

# Include dependency files
-include $(DEP)

# Clean build
clean:
	rm -rf $(OBJ_DIR) $(DEP) $(TARGET)

# Run
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
