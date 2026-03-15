CXX      = g++
CXXFLAGS = -Wall -Wextra -std=c++17 $(shell pkg-config --cflags sdl2)
LDFLAGS  = $(shell pkg-config --libs sdl2)

SRCS    = main.cpp 6502.cpp address.cpp bus.cpp instruction.cpp mapper.cpp mapper_000.cpp mapper_001.cpp mapper_003.cpp mapper_004.cpp ppu.cpp rom.cpp apu.cpp
OBJDIR  = build
OBJS    = $(SRCS:%.cpp=$(OBJDIR)/%.o)
TARGET  = nespc

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
