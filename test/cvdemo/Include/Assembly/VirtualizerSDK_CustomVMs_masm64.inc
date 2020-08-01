; ******************************************************************************
; Header: VirtualizerSDK_CustomVMs_masm64.inc
; Description: MASM64 macros definitions
;
; Author/s: Oreans Technologies 
; (c) 2013 Oreans Technologies
;
; --- File generated automatically from Oreans VM Generator (11/11/2013) ---
; ******************************************************************************

; ******************************************************************************
;                                 Macros definition
; ******************************************************************************

VIRTUALIZER_TIGER_WHITE_START MACRO

    db    0ebh, 10h
    dd    20205643h
    dd    103
    dd    0
    dd    20205643h
ENDM


VIRTUALIZER_TIGER_WHITE_END MACRO

    db    0ebh, 10h
    dd    20205643h
    dd    503
    dd    0
    dd    20205643h
ENDM


VIRTUALIZER_TIGER_RED_START MACRO

    db    0ebh, 10h
    dd    20205643h
    dd    104
    dd    0
    dd    20205643h
ENDM


VIRTUALIZER_TIGER_RED_END MACRO

    db    0ebh, 10h
    dd    20205643h
    dd    504
    dd    0
    dd    20205643h
ENDM


VIRTUALIZER_TIGER_BLACK_START MACRO

    db    0ebh, 10h
    dd    20205643h
    dd    105
    dd    0
    dd    20205643h
ENDM


VIRTUALIZER_TIGER_BLACK_END MACRO

    db    0ebh, 10h
    dd    20205643h
    dd    505
    dd    0
    dd    20205643h
ENDM


VIRTUALIZER_FISH_WHITE_START MACRO

    db    0ebh, 10h
    dd    20205643h
    dd    107
    dd    0
    dd    20205643h
ENDM


VIRTUALIZER_FISH_WHITE_END MACRO

    db    0ebh, 10h
    dd    20205643h
    dd    507
    dd    0
    dd    20205643h
ENDM


VIRTUALIZER_FISH_RED_START MACRO

    db    0ebh, 10h
    dd    20205643h
    dd    109
    dd    0
    dd    20205643h
ENDM


VIRTUALIZER_FISH_RED_END MACRO

    db    0ebh, 10h
    dd    20205643h
    dd    509
    dd    0
    dd    20205643h
ENDM


VIRTUALIZER_FISH_BLACK_START MACRO

    db    0ebh, 10h
    dd    20205643h
    dd    111
    dd    0
    dd    20205643h
ENDM


VIRTUALIZER_FISH_BLACK_END MACRO

    db    0ebh, 10h
    dd    20205643h
    dd    511
    dd    0
    dd    20205643h
ENDM


VIRTUALIZER_MUTATE_ONLY_START MACRO

    db    0ebh, 10h
    dd    20205643h
    dd    16
    dd    0
    dd    20205643h
ENDM


VIRTUALIZER_MUTATE_ONLY_END MACRO

    db    0ebh, 10h
    dd    20205643h
    dd    17
    dd    0
    dd    20205643h
ENDM


