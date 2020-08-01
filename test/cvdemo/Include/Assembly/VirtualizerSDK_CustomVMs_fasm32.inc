; ******************************************************************************
; Header: VirtualizerSDK_CustomVMs_fasm32.inc
; Description: FASM32 macros definitions
;
; Author/s: Oreans Technologies 
; (c) 2013 Oreans Technologies
;
; --- File generated automatically from Oreans VM Generator (11/11/2013) ---
; ******************************************************************************

; ******************************************************************************
;                                 Macros definition
; ******************************************************************************

macro VIRTUALIZER_TIGER_WHITE_START
{
    db    0ebh, 10h
    dd    20205643h
    dd    100
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_TIGER_WHITE_END
{
    db    0ebh, 10h
    dd    20205643h
    dd    500
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_TIGER_RED_START
{
    db    0ebh, 10h
    dd    20205643h
    dd    101
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_TIGER_RED_END
{
    db    0ebh, 10h
    dd    20205643h
    dd    501
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_TIGER_BLACK_START
{
    db    0ebh, 10h
    dd    20205643h
    dd    102
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_TIGER_BLACK_END
{
    db    0ebh, 10h
    dd    20205643h
    dd    502
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_FISH_WHITE_START
{
    db    0ebh, 10h
    dd    20205643h
    dd    106
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_FISH_WHITE_END
{
    db    0ebh, 10h
    dd    20205643h
    dd    506
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_FISH_RED_START
{
    db    0ebh, 10h
    dd    20205643h
    dd    108
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_FISH_RED_END
{
    db    0ebh, 10h
    dd    20205643h
    dd    508
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_FISH_BLACK_START
{
    db    0ebh, 10h
    dd    20205643h
    dd    110
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_FISH_BLACK_END
{
    db    0ebh, 10h
    dd    20205643h
    dd    510
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_MUTATE_ONLY_START
{
    db    0ebh, 10h
    dd    20205643h
    dd    16
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_MUTATE_ONLY_END
{
    db    0ebh, 10h
    dd    20205643h
    dd    17
    dd    0
    dd    20205643h
}


