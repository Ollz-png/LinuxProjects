# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wshadow -g `pkg-config --cflags gtk+-3.0 vte-2.91`
LDFLAGS = `pkg-config --libs gtk+-3.0 vte-2.91`

# Source files
SRC := $(wildcard src/*.cpp)
OBJ := $(SRC:.cpp=.o)
DEP := $(OBJ:.o=.d)   # dependency files

# Include path
INCLUDES = -Iinclude

# Target
TARGET = mini-t

# Default rule
all: $(TARGET)

# Compile object files with dependency generation
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -MMD -MP -c $< -o $@

# Link executable
$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)

# Include dependency files if they exist
-include $(DEP)

# Clean build
clean:
	rm -f $(OBJ) $(DEP) $(TARGET)

# Run
run: $(TARGET)
	./$(TARGET)
