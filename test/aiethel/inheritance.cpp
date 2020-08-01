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

void execute(){
    puts("Executed");
}

void executer(void (*ex)()){
    ex();
}

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



int main() {
    Person *p = new Calm(27);
    p->shout();
    executer(execute);
    delete p;
    return 0;
}
