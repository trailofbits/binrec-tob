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
VirtualizerMutate1Start         PROTO
VirtualizerMutate2Start         PROTO
VirtualizerMutate3Start         PROTO


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

VIRTUALIZER_MUTATE1_START MACRO 

    jmp     @F

    db      'CV  '
    dd      ID_VM_START
    dd      1
    db      'CV  '

    @@:

ENDM    

VIRTUALIZER_MUTATE2_START MACRO 

    jmp     @F

    db      'CV  '
    dd      ID_VM_START
    dd      2
    db      'CV  '

    @@:

ENDM    

VIRTUALIZER_MUTATE3_START MACRO 

    jmp     @F

    db      'CV  '
    dd      ID_VM_START
    dd      3
    db      'CV  '

    @@:

ENDM    

VIRTUALIZER1_START MACRO 

    jmp     @F

    db      'CV  '
    dd      ID_VM_START
    db      0
    db      1
    dw      0
    db      'CV  '

    @@:

ENDM    

VIRTUALIZER1_END MACRO 

    jmp     @F

    db      'CV  '
    dd      ID_VM_END
    dd      0
    db      'CV  '

    @@:

ENDM   

VIRTUALIZER2_START MACRO 

    jmp     @F

    db      'CV  '
    dd      ID_VM_START
    db      0
    db      2
    dw      0
    db      'CV  '

    @@:

ENDM    

VIRTUALIZER2_END MACRO 

    jmp     @F

    db      'CV  '
    dd      ID_VM_END
    dd      0
    db      'CV  '

    @@:

ENDM   

VIRTUALIZER3_START MACRO 

    jmp     @F

    db      'CV  '
    dd      ID_VM_START
    db      0
    db      3
    dw      0
    db      'CV  '

    @@:

ENDM    

VIRTUALIZER3_END MACRO 

    jmp     @F

    db      'CV  '
    dd      ID_VM_END
    dd      0
    db      'CV  '

    @@:

ENDM   

VIRTUALIZER4_START MACRO 

    jmp     @F

    db      'CV  '
    dd      ID_VM_START
    db      0
    db      4
    dw      0
    db      'CV  '

    @@:

ENDM    

VIRTUALIZER4_END MACRO 

    jmp     @F

    db      'CV  '
    dd      ID_VM_END
    dd      0
    db      'CV  '

    @@:

ENDM   

VIRTUALIZER5_START MACRO 

    jmp     @F

    db      'CV  '
    dd      ID_VM_START
    db      0
    db      5
    dw      0
    db      'CV  '

    @@:

ENDM    

VIRTUALIZER1_END MACRO 

    jmp     @F

    db      'CV  '
    dd      ID_VM_END
    dd      0
    db      'CV  '

    @@:

ENDM   

