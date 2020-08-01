How to add a Stealth Code area in your application
--------------------------------------------------

1) Include "StealthCodeArea.h" in one of your source code files

2) The "STEALTH_AUX_FUNCTION" define a simple function which receives
   a "int" parameter. 

3) Create a function *which is never executed* and insert there 
   the STEALTH AREA macros. Example:


int main(void)
{
   // we have to reference "MyFunctionStealthCodeArea" so the
   // compiler optimizations don't remove that function from being generated
   // as it's never called. So we create a condition that it's never true. 
   // Example:

   if ((void*)&MessageBox == (void*)&Sleep)
      MyStealthCodeArea(); 

}

// The following macro creates a simple function to be referenced by
// the Stealth area

STEALTH_AUX_FUNCTION

//
// Here is the function which contains the Stealth area
//
void MyFunctionStealthCodeArea(void)
{
    STEALTH_AREA_START

    // Here we create our Stealth area. Insert more entries if you
    // require more space for the protection code

    STEALTH_AREA_CHUNK
    STEALTH_AREA_CHUNK
    STEALTH_AREA_CHUNK
    STEALTH_AREA_CHUNK

    STEALTH_AREA_END
}


4) NOTE: As the function "MyFunctionStealthCodeArea" contains a lot of dummy code, 
         we recommend that you put it in a separate file to avoid recompiling that 
         code everytime that you make changes in your code.