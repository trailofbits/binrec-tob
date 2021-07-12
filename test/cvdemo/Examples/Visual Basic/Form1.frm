VERSION 5.00
Begin VB.Form Form1 
   Caption         =   "Macros Example"
   ClientHeight    =   2655
   ClientLeft      =   60
   ClientTop       =   345
   ClientWidth     =   5265
   LinkTopic       =   "Form1"
   ScaleHeight     =   2655
   ScaleWidth      =   5265
   StartUpPosition =   2  'CenterScreen
   Begin VB.Frame Frame1 
      Caption         =   "Encode Macro"
      Height          =   1815
      Left            =   600
      TabIndex        =   0
      Top             =   360
      Width           =   4215
      Begin VB.CommandButton Command4 
         Caption         =   "Execute Virtualizer Macro #2"
         Height          =   375
         Left            =   720
         TabIndex        =   2
         Top             =   1080
         Width           =   2655
      End
      Begin VB.CommandButton Command3 
         Caption         =   "Execute Virtualizer Macro #1"
         Height          =   375
         Left            =   720
         TabIndex        =   1
         Top             =   480
         Width           =   2655
      End
   End
End
Attribute VB_Name = "Form1"
Attribute VB_GlobalNameSpace = False
Attribute VB_Creatable = False
Attribute VB_PredeclaredId = True
Attribute VB_Exposed = False


Private Sub Command3_Click()

    ' the following code, inside the Virtualizer Macro will be
    ' converted into virtual opcodes
    
    Call VarPtr("VIRTUALIZER_START")
    
    MsgBox "The Virtualizer Macro #1 has successfully been executed", vbInformation + vbOKOnly, "Code Virtualizer example"
    
    Call VarPtr("VIRTUALIZER_END")
 
End Sub

Private Sub Command4_Click()

    ' the following code, inside the Virtualizer Macro will be
    ' converted into virtual opcodes
    
    Call VarPtr("VIRTUALIZER_START")
    
    MsgBox "The Virtualizer Macro #2 has successfully been executed", vbInformation + vbOKOnly, "Code Virtualizer example"
    
    Call VarPtr("VIRTUALIZER_END")
    
    
End Sub

