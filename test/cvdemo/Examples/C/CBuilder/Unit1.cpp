//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Unit1.h"
#include "..\..\..\Include\C\VirtualizerSDK.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TForm1 *Form1;
//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
        : TForm(Owner)
{
}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button1Click(TObject *Sender)
{
     int        value = 0;

     // the following code, inside the Virtualizer macro, will be
     // converted into Virtual opcodes

     VIRTUALIZER_TIGER_WHITE_START

     for (int i = 0; i < 10; i++)
     {
         value += value * i;
     }

     VIRTUALIZER_TIGER_WHITE_START

     MessageBox(NULL, "The Virtualizer Macro #1 has successfully been executed", "Code Virtualizer", MB_OK + MB_ICONINFORMATION);

}
//---------------------------------------------------------------------------

void __fastcall TForm1::Button2Click(TObject *Sender)
{
     int        value = 0;

     // the following code, inside the Virtualizer macro, will be
     // converted into Virtual opcodes

     VIRTUALIZER_TIGER_WHITE_START

     for (int i = 0; i < 10; i++)
     {
         value += value * i * 2;
     }

     VIRTUALIZER_TIGER_WHITE_START

     MessageBox(NULL, "The Virtualizer Macro #2 has successfully been executed", "Code Virtualizer", MB_OK + MB_ICONINFORMATION);

}
//---------------------------------------------------------------------------
