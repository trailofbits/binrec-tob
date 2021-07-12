// ----------------------------------------------------------------
// Example: How to create a Stealth Area in a PureBasicapplication
// ----------------------------------------------------------------


1) In any procedure/function in your source code, insert the Stealth 
   Area. Notice that the Stealth Area should never be executed, so we 
   can just create a condition that it's never true. Example:


XIncludeFile "StealthCodeArea.pbi"


always_true = 1

If always_true = 0  
  
  STEALTH_AREA_START

  STEALTH_AREA_CHUNK
  STEALTH_AREA_CHUNK
  STEALTH_AREA_CHUNK
  STEALTH_AREA_CHUNK

STEALTH_AREA_END

EndIf

;
; Your code here
;

