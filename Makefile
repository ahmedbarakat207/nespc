# NESPC Emulator Makefile
# Compiler
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 $(shell pkg-config --cflags sdl2)
LDFLAGS = $(shell pkg-config --libs sdl2)


# Source files
SRCS = main.cpp 6502.cpp address.cpp bus.cpp instruction.cpp mapper.cpp mapper_000.cpp ppu.cpp rom.cpp


# Object files in build directory
OBJDIR = build
OBJS = $(SRCS:%.cpp=$(OBJDIR)/%.o)

# Executable name
TARGET = nespc


all: $(OBJDIR) $(TARGET)


$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^


$(OBJDIR)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@


$(OBJDIR):
	mkdir -p $(OBJDIR)

clean:
	rm -rf $(OBJDIR) $(TARGET)

.PHONY: all clean
