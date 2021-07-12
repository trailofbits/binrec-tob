; **************************************************************************** 
; Module: Example.asm
; Description: Example program that shows the use of the Code Virtualizer macros
;              in assembly language
;
; Author/s: Rafael Ahucha 
; (c) 2006 Oreans Technologies
; **************************************************************************** 

.586
.model flat,stdcall
option casemap:none


; **************************************************************************** 
;                   Libraries used in this module
; **************************************************************************** 

include VirtualizerSDK.inc

include \masm32\include\windows.inc
include \masm32\include\kernel32.inc
include \masm32\include\user32.inc
includelib \masm32\lib\kernel32.lib
includelib \masm32\lib\user32.lib


; **************************************************************************** 
;                               Macros definition 
; **************************************************************************** 

literal MACRO quoted_text:VARARG

    LOCAL local_text

    .data
      local_text db quoted_text,0

    .code
    EXITM <local_text>

ENDM

SADD MACRO quoted_text:VARARG

    EXITM <ADDR literal(quoted_text)>

ENDM    


; **************************************************************************** 
;                                 Constants 
; **************************************************************************** 

.const


; **************************************************************************** 
;                                Global data
; **************************************************************************** 

.data


; **************************************************************************** 
;                                Code section
; **************************************************************************** 

.code

Start:
 
    VIRTUALIZER_START

    invoke  MessageBox, NULL, SADD("We are showing this message inside an Virtualizer macro"), SADD("Code Virtualizer"), MB_OK

    VIRTUALIZER_END

    VIRTUALIZER_MUTATE2_START

    invoke  MessageBox, NULL, SADD("We are showing this message inside an Virtualizer macro with mutation level 2"), SADD("Code Virtualizer"), MB_OK

    VIRTUALIZER_END

    invoke  MessageBox, NULL, SADD("Virtualizer macro executed OK!"), SADD("Code Virtualizer"), MB_OK

    invoke  ExitProcess, 0
    
end Start
