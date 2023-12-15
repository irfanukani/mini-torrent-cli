# Compiler and compiler flags
CXX = g++
CXXFLAGS = -std=c++11 -Wall

# Target executable
TARGET = tracker

# Source files
SRCS = tracker.cpp utilities.cpp handlers.cpp

# Object files (generated from source files)
OBJS = $(SRCS:.cpp=.o)

# Header files
HEADERS = tracker.h

# Default target
all: $(TARGET)

# Linking step to generate the executable
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Compilation step for each source file
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean the generated files
clean:
	rm -f $(OBJS) $(TARGET)

# Phony target to run the program
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run
