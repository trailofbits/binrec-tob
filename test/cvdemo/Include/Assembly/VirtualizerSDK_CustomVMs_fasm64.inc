; ******************************************************************************
; Header: VirtualizerSDK_CustomVMs_fasm64.inc
; Description: FASM64 macros definitions
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
    dd    103
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_TIGER_WHITE_END
{
    db    0ebh, 10h
    dd    20205643h
    dd    503
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_TIGER_RED_START
{
    db    0ebh, 10h
    dd    20205643h
    dd    104
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_TIGER_RED_END
{
    db    0ebh, 10h
    dd    20205643h
    dd    504
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_TIGER_BLACK_START
{
    db    0ebh, 10h
    dd    20205643h
    dd    105
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_TIGER_BLACK_END
{
    db    0ebh, 10h
    dd    20205643h
    dd    505
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_FISH_WHITE_START
{
    db    0ebh, 10h
    dd    20205643h
    dd    107
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_FISH_WHITE_END
{
    db    0ebh, 10h
    dd    20205643h
    dd    507
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_FISH_RED_START
{
    db    0ebh, 10h
    dd    20205643h
    dd    109
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_FISH_RED_END
{
    db    0ebh, 10h
    dd    20205643h
    dd    509
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_FISH_BLACK_START
{
    db    0ebh, 10h
    dd    20205643h
    dd    111
    dd    0
    dd    20205643h
}


macro VIRTUALIZER_FISH_BLACK_END
{
    db    0ebh, 10h
    dd    20205643h
    dd    511
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


