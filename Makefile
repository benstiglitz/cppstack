CXX = g++
CCOPTS=-g

all: stack

clean:
	rm -f *.o; rm -rf *.dSYM; rm -f stack

stack: stack.cpp compiler.o dictionary.o compiler.h types.h
	$(CXX) $(CCOPTS) -o stack -lreadline stack.cpp compiler.o dictionary.o

compiler.o: compiler.cpp compiler.h types.h
	$(CXX) $(CCOPTS) -o compiler.o -c compiler.cpp

dictionary.o: dictionary.cpp dictionary.h types.h
	$(CXX) $(CCOPTS) -o dictionary.o -c dictionary.cpp
