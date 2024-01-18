#include "perfevents.hpp"


int main()
{
    perfevents pe;
    pe.start("Start");
    printf("Hello World!\n");
    pe.stop("Stop");

    return 0;
}

