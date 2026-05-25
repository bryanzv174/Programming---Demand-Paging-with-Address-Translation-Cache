
# Carlos D. Martinez                                Red ID: 827940172
# Bryan D. Zavala Velasco                      Red ID: 130177824


# C++ compiler
CXX = g++

# Compiler flags:
# -Wall enables all warnings
# -std=c++14 specifies C++14 standard
CXXFLAGS = -Wall -std=c++14

# Source files for the program
SRCS = main.cpp pagetable.cpp tlb.cpp tracereader.c log.cpp

# Object files automatically derived from source files
OBJS = $(SRCS:.cpp=.o)

# Name of the final executable
EXEC = pagingwithatc

# Default target to build the executable
all: $(EXEC)

# Link object files to create the final executable
$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(EXEC) $(OBJS)

# Compile .cpp files into .o files (general rule for any .cpp file)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up all object files and the executable
clean:
	rm -f $(OBJS) $(EXEC)
