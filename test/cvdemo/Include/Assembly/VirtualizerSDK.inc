;  
; Module: Virtualizer.inc
;
; Description: Macros definitions for Virtualizer SDK (asm)
;
; Authors: Rafael Ahucha
;
; (c) 2006 Oreans Technologies 
;  


;  
;                                Functions Declaration
;  

VirtualizerStart                PROTO 
VirtualizerEnd                  PROTO


;  
;                                General Consts
;  

ID_VM_START                     EQU     12
ID_VM_END                       EQU     13



;  
;                                Macros definitions
;  


VIRTUALIZER_START MACRO 

    jmp     @F

    db      'CV  '
    dd      ID_VM_START
    dd      0
    db      'CV  '

    @@:

ENDM    

VIRTUALIZER_END MACRO 

    jmp     @F

    db      'CV  '
    dd      ID_VM_END
    dd      0
    db      'CV  '

    @@:

ENDM   

