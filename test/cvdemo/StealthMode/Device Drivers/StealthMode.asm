; ****************************************************************************
; Module: StealthMode.asm
; Description: Assembly Language module to leave space for Stealth Area
;
; Author/s: Rafael Ahucha 
; (c) 2013 Oreans Technologies
; ****************************************************************************

; ****************************************************************************
;                                   Model
; ****************************************************************************

IFDEF RAX

ELSE

.586
.model flat,stdcall
option casemap:none

ENDIF


; ****************************************************************************
;                                 Constants
; ****************************************************************************

.CONST


; ****************************************************************************
;                                  Macros
; ****************************************************************************

STEALTH_AREA_START  MACRO 

    mov     eax, 0A1A2A3A4h
    mov     ebx, 0A4A3A2A1h
    mov     ecx, 0B1A1B2A2h
    mov     edx, 0B8A8A1A1h

ENDM

STEALTH_AREA_END    MACRO 

    mov     eax, 0B6B5B3B6h
    mov     ebx, 0A2B2C2D2h
    mov     ecx, 0A9A8A2A2h
    mov     edx, 0A0A9B9B8h

ENDM

STEALTH_AREA_256KB_CHUNK   MACRO

    db 40000h dup (0)

ENDM


; ****************************************************************************
;                               Data Segment
; ****************************************************************************

.DATA


; ****************************************************************************
;                               Code Segment
; ****************************************************************************

.CODE

; ****************************************************************************
; * StealthAreaFunction 
; *
; *  This function should never be called. It just contains dummy code which 
; *  will be replaced by Code Virtualizer protection code
; *
; * Inputs
; *  None
; * 
; * Outputs
; *  None
; *
; * Returns
; *  None
; *****************************************************************************

StealthAreaFunction proc

    STEALTH_AREA_START

    ; you can increase the Stealth Area by adding "STEALTH_AREA_256KB_CHUNK" 
    ; code lines. The following code contains 8 * 256Kb = 2Mb Stealth Area
    
    STEALTH_AREA_256KB_CHUNK
    STEALTH_AREA_256KB_CHUNK
    STEALTH_AREA_256KB_CHUNK
    STEALTH_AREA_256KB_CHUNK
    STEALTH_AREA_256KB_CHUNK
    STEALTH_AREA_256KB_CHUNK
    STEALTH_AREA_256KB_CHUNK
    STEALTH_AREA_256KB_CHUNK

    STEALTH_AREA_END

StealthAreaFunction endp

end
