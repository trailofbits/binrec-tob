#define _XOPEN_SOURCE
#include <setjmp.h>
#include <stdio.h>

int main()
{
  sigjmp_buf env;
  int sigMask = 0;
  int i = 0;

  i = sigsetjmp(env, sigMask);
  printf("i = %d\n", i);
  if (i == 5) return 0;
  if (i == 0)
     siglongjmp(env, 2);
  if (i == 2)
     siglongjmp(env, 4);
  if (i == 4)
     siglongjmp(env, 5);
  printf("Does this line get printed?\n");
  return 0;
}
