#include <stdio.h>
#include <stack>
#include <vector>
#include <memory>
#include <iostream>
#include <fstream>
#include "types.h"
#include "Compiler.h"

const int STACK_SIZE = 1024;
//std::deque<Value> master_stack;
Value *stack_bottom, *stack_top;
std::stack<Frame> return_stack;
std::vector<void (*)(void)> prims;

void die(char *msg) {
    printf("die: %s\n", msg);
    exit(1);
}

void push(Value v) {
    *stack_top++ = v;
}

Value pop() {
    if (stack_top == stack_bottom) {
	die("Stack underflow");
    }
    Value v = *--stack_top;
    return v;
}

Value call(Clause *c) {
    return_stack.push(std::make_pair(c, 0));
    Frame &f = return_stack.top();
    while(f.second < f.first->size()) {
	Value v = (*f.first)[f.second];
	if ((v & 3) == 1) {
	    push(v >> 2);
	} else if ((v & 3) == 2) {
	    push(v & (~2));
	} else if ((v & 3) == 3) {
	    prims[v>>2]();
	} else {
	    call((Clause *)v);
	}
	f.second++;
    }
}


void op_if() {
    Value false_clause = pop(), true_clause = pop(), v = pop();
    Value clause = (v != 0) ? true_clause : false_clause;
    Clause *c = (Clause *)clause;
    call(c);
}

void op_print() {
    std::cout << pop() << std::endl;
}

void op_add() {
    push(pop() + pop());
}

void op_sub() {
    int b = pop(), a = pop();
    push(a - b);
}

void op_call() {
    call((Clause *)pop());
}

void op_while() {
    Value loop = pop(), predicate = pop();
    call((Clause *)predicate);
    while (pop()) {
	call((Clause *)loop);
	call((Clause *)predicate);
    }
}

void op_greater() {
    int b = pop(), a = pop();
    push(a > b ? 1 : 0);
}

void op_store() {
    Value *addr = (Value *)pop();
    Value v = pop();
    *addr = v;
}

void op_load() {
    Value *addr = (Value *)pop();
    push(*addr);
}

void op_drop() {
    (void)pop();
}

void op_pick() {
    Value index = pop();
    //index = master_stack.size() - index - 1;
    index = stack_top - stack_bottom - index - 1;
    if (index < 0) {
	die("pick underflow");
    }
    //push(master_stack[index]);
    push(stack_bottom[index]);
}

void op_swap() {
    Value a = pop(), b = pop();
    push(a); push(b);
}

void op_rot() {
    Value c = pop(), b = pop(), a = pop();
    push(b); push(c); push(a);
}

void op_not() {
    push(!pop());
}

void op_stack_bottom() {
    push((Value)stack_bottom);
}

void op_stack_top() {
    push((Value)stack_top);
}

void op_mul() {
  push(pop() * pop());
}

void op_divmod() {
  Value b = pop(), a = pop();
  push(a / b);
  push(a % b);
}

int main(int argc, char **argv) {
    stack_bottom = stack_top = (Value *)calloc(sizeof(Value), STACK_SIZE);

    prims.push_back(op_if);
    prims.push_back(op_print);
    prims.push_back(op_add);
    prims.push_back(op_sub);
    prims.push_back(op_call);
    prims.push_back(op_while);
    prims.push_back(op_greater);
    prims.push_back(op_store);
    prims.push_back(op_load);
    prims.push_back(op_pick);
    prims.push_back(op_swap);
    prims.push_back(op_rot);
    prims.push_back(op_not);
    prims.push_back(op_drop);
    prims.push_back(op_stack_top);
    prims.push_back(op_stack_bottom);
    prims.push_back(op_mul);
    prims.push_back(op_divmod);

    Compiler c;
    std::string input;
    
    // Read from file
    std::ifstream f;
    f.open("stdlib.f");
    while(!std::getline(f, input).eof()) {
	call(c.compile(input));
    }

    // Read from standard input
    std::cout << "> "; std::getline(std::cin, input);
    while (!input.empty()) {
	call(c.compile(input));
	std::cout << "> "; std::getline(std::cin, input);
    }
    return 0;
}
