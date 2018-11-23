CXX = g++
CXXFLAGSLINUX = $(WARNINGS) -std=c++11 -ggdb3 -static
CXXFLAGSOSX = $(WARNINGS) -std=c++11 -ggdb3
CXXFLAGSWIN = $(WARNINGS) -std=gnu++11 -ggdb3 -static
WARNINGS = -Wall -Wextra -pedantic
LFLAGS = -std=c++11
LFLAGSLINUX = -std=c++11 -static
LFLAGSWIN = -std=gnu++11 -static

# Operating System
OS := $(shell uname)

CPPS = $(wildcard *.cc)
HEADERS = $(wildcard *.h)
OBJS = $(CPPS:%.cc=%.o)
TARGET = modrev


default: all

.PHONY: all clean depend

all: depend $(TARGET)

$(TARGET): $(OBJS)
ifeq ($(OS),Linux)
	$(CXX) $(LFLAGSLINUX) -o $@ $^
else ifeq ($(OS),Darwin)
	$(CXX) $(LFLAGS) -o $@ $^
else
	$(CXX) $(LFLAGSWIN) -o $@ $^
endif

%.o: %.cc
ifeq ($(OS),Linux)
	$(CXX) $(CXXFLAGSLINUX) -c $<
else ifeq ($(OS),Darwin)
	$(CXX) $(CXXFLAGSOSX) -c $<
else
	$(CXX) $(CXXFLAGSWIN) -c $<
endif

clean:
ifeq ($(OS),Linux)
	rm -f .makedeps *.o $(TARGET)
else ifeq ($(OS),Darwin)
	rm -f .makedeps *.o $(TARGET)
else
	rm .makedeps 
	rm *.o 
	rm $(TARGET).exe
endif

depend: $(CPPS) $(HEADERS)
ifeq ($(OS),Linux)
	$(CXX) $(CXXFLAGSLINUX) -MM $(CPPS) > .makedeps
else ifeq ($(OS),Darwin)
	$(CXX) $(CXXFLAGSOSX) -MM $(CPPS) > .makedeps
else
	$(CXX) $(CXXFLAGSWIN) -MM $(CPPS) > .makedeps
endif
