#include "userlib.hpp"
#include "stdio.h"

class Stack
{
  private:
    int* stack;
    int p;
  public:
    Stack(size_t max = 500);
    ~Stack();
    void push(int value);
    int pop();
    bool empty() const;
};

Stack::Stack(size_t max)
    : stack(new int[max]), p(0)
{
}
Stack::~Stack()
{
    delete stack;
}
void Stack::push(int value)
{
    stack[p++] = value;
}
int Stack::pop()
{
    return stack[--p];
}
bool Stack::empty() const
{
    return !p;
}


int main()
{
    setScrollField(0, 43); // The train should not be destroyed by the output, so we shrink the scrolling area...
    printLine("================================================================================", 0, 0x0B);
    printLine("                               C++ - Testprogramm!                              ", 2, 0x0B);
    printLine("--------------------------------------------------------------------------------", 4, 0x0B);
    printLine("                                 ! Hello World !                                ", 7, 0x0C);

    Stack acc(500);

    iSetCursor(0,6);

    for(int i=0; i<500; i++)
    {
        acc.push(i);
        showInfo(1);
    }
    for(int i=0; i<500; i++)
    {
        printf("%u ", acc.pop());
        showInfo(1);
    }
    for(;;);
    return(0);
}
