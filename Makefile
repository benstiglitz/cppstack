CXX = g++
CCOPTS=-g

all: stack

clean:
	rm -f *.o; rm -rf *.dSYM; rm -f stack

stack: stack.cpp compiler.o compiler.h types.h
	$(CXX) $(CCOPTS) -o stack -lreadline stack.cpp compiler.o

compiler.o: compiler.cpp compiler.h types.h
	$(CXX) $(CCOPTS) -o compiler.o -c compiler.cpp
