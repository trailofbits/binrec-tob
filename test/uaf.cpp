//known violation with asan, binrec catches
int main(int argc, char **argv) {
      int *array = new int[100];
            delete [] array;
              return array[argc];

}
