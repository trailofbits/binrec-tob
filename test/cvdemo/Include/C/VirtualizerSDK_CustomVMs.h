/******************************************************************************
 * Header: VirtualizerSDK_CustomVMs.h
 * Description:  Definitions for Custom VMs in SecureEngine
 *
 * Author/s: Oreans Technologies 
 * (c) 2013 Oreans Technologies
 *
 * --- File generated automatically from Oreans VM Generator (11/11/2013) ---
 ******************************************************************************/

// ***********************************************
// Definition of macros as function names
// ***********************************************

 #ifdef __cplusplus
  extern "C" {
 #endif

__declspec(dllimport) void __stdcall CustomVM00000100_Start(void);

__declspec(dllimport) void __stdcall CustomVM00000100_End(void);

__declspec(dllimport) void __stdcall CustomVM00000103_Start(void);

__declspec(dllimport) void __stdcall CustomVM00000103_End(void);

__declspec(dllimport) void __stdcall CustomVM00000101_Start(void);

__declspec(dllimport) void __stdcall CustomVM00000101_End(void);

__declspec(dllimport) void __stdcall CustomVM00000104_Start(void);

__declspec(dllimport) void __stdcall CustomVM00000104_End(void);

__declspec(dllimport) void __stdcall CustomVM00000102_Start(void);

__declspec(dllimport) void __stdcall CustomVM00000102_End(void);

__declspec(dllimport) void __stdcall CustomVM00000105_Start(void);

__declspec(dllimport) void __stdcall CustomVM00000105_End(void);

__declspec(dllimport) void __stdcall CustomVM00000106_Start(void);

__declspec(dllimport) void __stdcall CustomVM00000106_End(void);

__declspec(dllimport) void __stdcall CustomVM00000107_Start(void);

__declspec(dllimport) void __stdcall CustomVM00000107_End(void);

__declspec(dllimport) void __stdcall CustomVM00000108_Start(void);

__declspec(dllimport) void __stdcall CustomVM00000108_End(void);

__declspec(dllimport) void __stdcall CustomVM00000109_Start(void);

__declspec(dllimport) void __stdcall CustomVM00000109_End(void);

__declspec(dllimport) void __stdcall CustomVM00000110_Start(void);

__declspec(dllimport) void __stdcall CustomVM00000110_End(void);

__declspec(dllimport) void __stdcall CustomVM00000111_Start(void);

__declspec(dllimport) void __stdcall CustomVM00000111_End(void);

__declspec(dllimport) void __stdcall Mutate_Start(void);

__declspec(dllimport) void __stdcall Mutate_End(void);

__declspec(dllimport) void __stdcall Mutate_Start(void);

__declspec(dllimport) void __stdcall Mutate_End(void);

#ifdef __cplusplus
}
#endif


// ***********************************************
// x64 definition as function names
// ***********************************************

#if defined(PLATFORM_X64) && !defined(CV_X64_INSERT_VIA_INLINE)

#define VIRTUALIZER_TIGER_WHITE_START CustomVM00000103_Start();
#define VIRTUALIZER_TIGER_WHITE_END CustomVM00000103_End();

#define VIRTUALIZER_TIGER_RED_START CustomVM00000104_Start();
#define VIRTUALIZER_TIGER_RED_END CustomVM00000104_End();

#define VIRTUALIZER_TIGER_BLACK_START CustomVM00000105_Start();
#define VIRTUALIZER_TIGER_BLACK_END CustomVM00000105_End();

#define VIRTUALIZER_FISH_WHITE_START CustomVM00000107_Start();
#define VIRTUALIZER_FISH_WHITE_END CustomVM00000107_End();

#define VIRTUALIZER_FISH_RED_START CustomVM00000109_Start();
#define VIRTUALIZER_FISH_RED_END CustomVM00000109_End();

#define VIRTUALIZER_FISH_BLACK_START CustomVM00000111_Start();
#define VIRTUALIZER_FISH_BLACK_END CustomVM00000111_End();

#define VIRTUALIZER_MUTATE_ONLY_START Mutate_Start();
#define VIRTUALIZER_MUTATE_ONLY_END Mutate_End();

#define CV_CUSTOM_VMS_DEFINED

#endif 


// ***********************************************
// x32 definition as function names
// ***********************************************

#if defined(PLATFORM_X32) && !defined(CV_X32_INSERT_VIA_INLINE)

#define VIRTUALIZER_TIGER_WHITE_START CustomVM00000100_Start();
#define VIRTUALIZER_TIGER_WHITE_END CustomVM00000100_End();

#define VIRTUALIZER_TIGER_RED_START CustomVM00000101_Start();
#define VIRTUALIZER_TIGER_RED_END CustomVM00000101_End();

#define VIRTUALIZER_TIGER_BLACK_START CustomVM00000102_Start();
#define VIRTUALIZER_TIGER_BLACK_END CustomVM00000102_End();

#define VIRTUALIZER_FISH_WHITE_START CustomVM00000106_Start();
#define VIRTUALIZER_FISH_WHITE_END CustomVM00000106_End();

#define VIRTUALIZER_FISH_RED_START CustomVM00000108_Start();
#define VIRTUALIZER_FISH_RED_END CustomVM00000108_End();

#define VIRTUALIZER_FISH_BLACK_START CustomVM00000110_Start();
#define VIRTUALIZER_FISH_BLACK_END CustomVM00000110_End();

#define VIRTUALIZER_MUTATE_ONLY_START Mutate_Start();
#define VIRTUALIZER_MUTATE_ONLY_END Mutate_End();

#define CV_CUSTOM_VMS_DEFINED

#endif 


// ***********************************************
// x32/x64 definition as inline assembly
// ***********************************************

#ifndef CV_CUSTOM_VMS_DEFINED

#ifdef __BORLANDC__
  #include "VirtualizerSDK_CustomVMs_BorlandC_inline.h"
#endif

#ifdef __GNUC__
  #include "VirtualizerSDK_CustomVMs_GNU_inline.h"
#endif

#ifdef __ICL
  #include "VirtualizerSDK_CustomVMs_ICL_inline.h"
#endif

#ifdef __LCC__
  #include "VirtualizerSDK_CustomVMs_LCC_inline.h"
#endif

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
  #include "VirtualizerSDK_CustomVMs_VC_inline.h"
#endif

#endif
