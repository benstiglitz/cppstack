CXX = g++
CCOPTS=-g

all: stack

clean:
	rm -f *.o; rm -rf *.dSYM; rm -f stack

stack: stack.cpp compiler.o
	$(CXX) $(CCOPTS) -o stack stack.cpp compiler.o

compiler.o: compiler.cpp
	$(CXX) $(CCOPTS) -o compiler.o -c compiler.cpp
