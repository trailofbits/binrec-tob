// ----------------------------------------------------------------
// Example: How to create a Stealth Area in a Delphi application
// ----------------------------------------------------------------


1) The "StealthAuxFunction.inc" defines a simple function which 
   receives a "LongWord" parameter.


2) In any procedure/function in your source code, insert the Stealth 
   Area. Notice that the Stealth Area should never be executed, so we 
   can just create a condition that it's never true. Example:


// Define the stealth aux function which will receive references
// from the Stealth Area

{$I StealthAuxFunction.inc}


// in any function we can insert the Stealth Area
procedure MyFunction();

var
  always_false: Boolean;

begin

  always_false := False;    

  // As you can see, the Stealth Area is never executed. It will be filled
  // at a later time with protection code
  
  if (always_false = True) then  
  begin

    // Here we create our Stealth area. Insert more entries if you
    // require more space for the protection code

    {$I StealthArea_Start.inc}
    
    {$I StealthArea_Chunk.inc}
    {$I StealthArea_Chunk.inc}
    {$I StealthArea_Chunk.inc}
    {$I StealthArea_Chunk.inc}

    {$I StealthArea_End.inc}

  end;

  //
  // Your code here
  //
  
end;
