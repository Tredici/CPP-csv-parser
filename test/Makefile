CC:=g++
CPPFLAGS:=-ggdb -Wall -Wextra
EXE:=

all: run

run: main
	./main

EXE+=main
FILES=main.cc
DEPS=$(FILES:.cc=.d)
OBJS=$(FILES:.cc=.o)

main: $(OBJS)

%.d: %.cc
	$(CC) -MM -MF $@ $<
-include $(DEPS)

clean:
	rm -f $(EXE) *.o *.d
