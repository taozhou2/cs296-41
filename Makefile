EXE = debugger
OBJS += debug.o breakpoint.o
WARNINGS = -Wall -Werror -Wextra -Wno-unused-parameter -Wno-unused-variable

CXX = gcc
CXXFLAGS = -c -g -O0 $(WARNINGS)
LD = gcc 
LDFLAGS = -lpthread -lm

.PHONY: all $(EXE) clean

all: $(EXE)

$(EXE): $(OBJS)
	$(LD) $(OBJS) $(LDFLAGS) -o $(EXE)

debug.o: debug.c debug.h breakpoint.o
	$(CXX) $(CXXFLAGS) debug.c

breakpoint.o: breakpoint.c breakpoint.h
	$(CXX) $(CXXFLAGS) breakpoint.c

clean:
	rm -rf *.o $(EXE)