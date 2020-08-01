#include<stdio.h>

int i = 1;
int main(int argc, char *argv[])
{
//    int i = 1;
    printf("%x\n",&i);
    printf("%d\n",argc);
    for(i=0;i<argc;i++)
    {
        printf("%s\n",argv[i]);
        printf("%x\n",argv[i]);
    }
    return 0;
}
