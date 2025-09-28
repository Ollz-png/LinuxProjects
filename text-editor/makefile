# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wshadow -g `pkg-config --cflags gtk+-3.0 vte-2.91 gtksourceview-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0 vte-2.91 gtksourceview-3.0`

# Directories
SRC_DIR = src
OBJ_DIR = obj
INCLUDE_DIR = include

# Sources/Objects/Deps
SRCS := $(wildcard $(SRC_DIR)/*.cpp)
OBJ := $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))
DEP := $(OBJ:.o=.d)

# Target executable
TARGET = text-edit

# Default target
all: $(TARGET)

# Compile with dependency generation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -MMD -MP -c $< -o $@

# Ensure obj folder exists
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# Link
$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)

# Include dependency files
-include $(DEP)

# Clean
clean:
	rm -rf $(OBJ_DIR) $(DEP) $(TARGET)

# Run
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
