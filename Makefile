# Compiler
CXX = g++

# Target executable name
TARGET = eshell

# Source files
SRCS = eshell.cpp parser.c

# Compile and link the target
all:
	$(CXX) -o $(TARGET) $(SRCS)

# Clean target
clean:
	rm -f $(TARGET)
