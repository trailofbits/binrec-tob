/****************************************************************************** 
/* Module: Vc_example
/* Description: Shows how to call Code Virtualizer Macros in Visual C++ language
/*
/* Authors: Rafael Ahucha  
/* (c) 2013 Oreans Technologies
/*****************************************************************************/ 


/****************************************************************************** 
/*                   Libraries used by this module
/*****************************************************************************/ 

#include "stdafx.h"
#include <stdio.h>
#include "Resource.h"
#include <commctrl.h>
#include <windows.h>
#include "..\..\..\Include\C\Via ASM module\VirtualizerSDKMacros.h"


/****************************************************************************** 
/*                          Function prototypes
/*****************************************************************************/ 

LRESULT CALLBACK  MainHandler(HWND, UINT, WPARAM, LPARAM);


/****************************************************************************** 
/*                          Global variables
/*****************************************************************************/ 


/*****************************************************************************
* WinMain
*
*  Main program function
*
* Inputs
*  Standard WinMain parameters
* 
* Outputs
*  None
*
* Returns
*  Exit Code
******************************************************************************/

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
    DialogBox(GetModuleHandle(NULL), (LPCTSTR)IDD_ABOUTBOX, NULL, (DLGPROC)MainHandler);

    return 0;
}


/*****************************************************************************
* MainHandler
*
*  Message handler for dialog box
*
* Inputs
*  Standard Dlgbox message handle parameters
* 
* Outputs
*  None
*
* Returns
*  Exit Code
******************************************************************************/

LRESULT CALLBACK MainHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
   int value = 0; 

    switch (message)
    {

    case WM_INITDIALOG:
        
        return TRUE;

    case WM_COMMAND:

        if (LOWORD(wParam) == IDCANCEL) 
        {
			EndDialog(hDlg, LOWORD(wParam));
            return TRUE;
        }

        if (LOWORD(wParam) == IDC_BUTTON_ENCODE1)
        {
            // the following code, inside the VIRTUALIZER macro, will be converted
			// into virtual opcodes
      
            VIRTUALIZER_TIGER_WHITE_START

            for (int i = 0; i < 10; i++)
            {
                value += value * i;
            }

            MessageBox(NULL, "This is the Virtualizer macro #1", "Virtualizer Macro", MB_OK + MB_ICONINFORMATION);

            VIRTUALIZER_TIGER_WHITE_END
		}
        else if (LOWORD(wParam) == IDC_BUTTON_ENCODE2)
        { 
            // the following code, inside the VIRTUALIZER macro, will be converted
			// into virtual opcodes
      
            VIRTUALIZER_TIGER_WHITE_START

            for (int i = 0; i < 10; i++)
            {
                value += value * i * 3;
            }

			MessageBox(NULL, "This is the Virtualizer macro #2", "Virtualizer Macro", MB_OK + MB_ICONINFORMATION);

            VIRTUALIZER_TIGER_WHITE_END
		}
	}  
    return FALSE;
}
