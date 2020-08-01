/*
 * Copyright (c) 2019 Trail of Bits, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#include <memory>
//#include <string>
#include <iostream>

struct Person {
protected:
    unsigned age;
public:
    Person( unsigned old ) : age( old ) {
        puts("Person created");
    }

    virtual ~Person() {
        puts("Person destroyed");
    }

    virtual void shout() = 0;
    
    void incAge() {
       ++age;
       puts("age incremented");
    }
};

struct Calm : Person {
    Calm( unsigned age ) : Person( age + 10 ) {
        puts("Calm person created");
    }

    virtual ~Calm() {
        puts("Calm person destroyed");
    }

    void shout() override {
        puts("I am calm, can't shout");
    }

};

class MyArray {
protected:
    int *int_array = nullptr;
public:
    size_t size;
    MyArray(size_t s) : int_array(new int(s)), size(s){
        puts("MyArray created");
    }

    MyArray(){
        MyArray(10);
    }
    
    virtual ~MyArray() {
        delete[] int_array;
        puts("MyArray destroyed");
    }

    void operator delete[](void *ptr){
        puts("Custom delete[] is called");
        ::operator delete[](ptr);
    }
    
    void operator delete(void *ptr){
        puts("Custom delete is called");
        ::operator delete(ptr);
    }

    void insert(int i) {
       int_array[0] = i;
    }
};


int main(int argc, char **argv) {
    MyArray *ptr_array = new MyArray[2];

    MyArray *arr = new MyArray(argv[1][0] - '0');
    arr->insert(argv[2][0] - '0');
    printf("size: %d \n", arr->size);
    ptr_array[0] = *arr;
//    delete arr;

    arr = new MyArray(argv[2][0] - '0');
    arr->insert(argv[1][0] - '0');
    printf("size: %d \n", arr->size);
    ptr_array[1] = *arr;
//    delete arr;

//    delete ptr_array[0];
//    delete ptr_array[1];
    delete[] ptr_array;

/*    
    MyArray array[2];
    MyArray *arr = new MyArray(argv[1][0] - '0');
    arr->insert(argv[2][0] - '0');
    printf("size: %d \n", arr->size);
    array[0] = *arr;
    delete arr;
    arr = new MyArray(argv[2][0] - '0');
    arr->insert(argv[1][0] - '0');
    printf("size: %d \n", arr->size);
    array[1] = *arr;
    delete arr;
*/
    return 0;
}
