#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

//extern void __real__init();

void  __wrap__init()
{
    //printf("_init wrapper called \n");
    //int address = 0x1245678;
    asm ("push %[a]\n\t"
        "ret"
        :
        :[a]"i" (0xDEADBEEF)
        );
    //    return __real__init();
}
