#include <stdio.h>
//known violation with asan, binrec asan does not catch
      int stk[100] = {-1};
int main(int argc, char **argv) {
		puts("why");
              return stk[argc+100];

}
