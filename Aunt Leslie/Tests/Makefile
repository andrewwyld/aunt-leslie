# Makefile for Rotor Test Harness

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I../Source
LDFLAGS = -lm

# Source files
ROTOR_SRC = ../Source/Rotor.cpp
TEST_SRC = RotorTestHarness.cpp

# Object files
ROTOR_OBJ = Rotor.o
TEST_OBJ = RotorTestHarness.o

# Executable
TARGET = RotorTestHarness

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(ROTOR_OBJ) $(TEST_OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(ROTOR_OBJ) $(TEST_OBJ) $(LDFLAGS)

# Compile Rotor
$(ROTOR_OBJ): $(ROTOR_SRC) ../Source/Rotor.h
	$(CXX) $(CXXFLAGS) -c $(ROTOR_SRC) -o $(ROTOR_OBJ)

# Compile test harness
$(TEST_OBJ): $(TEST_SRC) ../Source/Rotor.h
	$(CXX) $(CXXFLAGS) -c $(TEST_SRC) -o $(TEST_OBJ)

# Run the tests
test: $(TARGET)
	./$(TARGET)

# Clean up
clean:
	rm -f $(ROTOR_OBJ) $(TEST_OBJ) $(TARGET)

.PHONY: all test clean
