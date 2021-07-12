/****************************************************************************** 
  Header: VirtualizerSDK.h
  Description: SDK header definition for the C/C++ language

  Author/s: Oreans Technologies  
  (c) 2011 Oreans Technologies
*****************************************************************************/ 

#pragma once


// ***********************************************
// Specify platform
// ***********************************************

#ifdef _WIN64
#define PLATFORM_X64
#else
#define PLATFORM_X32
#endif


// ***********************************************
// Defines
// ***********************************************

#define CV_X32_INSERT_VIA_INLINE
//#define CV_X64_INSERT_VIA_INLINE


// ***********************************************
// Include files
// ***********************************************

#include "VirtualizerSDK_CustomVMs.h"


// ***********************************************
// link with correct platform library
// ***********************************************

#ifndef CV_X64_INSERT_VIA_INLINE
 #ifdef PLATFORM_X64
  #ifdef _NTDDK_
    #pragma comment(lib, "VirtualizerDDK.lib")
  #else
    #pragma comment(lib, "VirtualizerSDK64.lib")
  #endif
 #endif
#endif

#ifndef CV_X32_INSERT_VIA_INLINE
 #ifdef PLATFORM_X32
  #ifdef _NTDDK_
    #pragma comment(lib, "VirtualizerDDK.lib")
  #else
    #pragma comment(lib, "VirtualizerSDK32.lib")
  #endif
 #endif
#endif


// ***********************************************
// In latest CV versions, we have removed the
// VirtualizerXStart and VirtualizerMutate macros
// ***********************************************

#define Virtualizer1Start           VirtualizerStart
#define Virtualizer2Start           VirtualizerStart
#define Virtualizer3Start           VirtualizerStart
#define Virtualizer4Start           VirtualizerStart
#define Virtualizer5Start           VirtualizerStart
#define Virtualizer1End             VirtualizerEnd
#define Virtualizer2End             VirtualizerEnd
#define Virtualizer3End             VirtualizerEnd
#define Virtualizer4End             VirtualizerEnd
#define Virtualizer5End             VirtualizerEnd
#define VirtualizerMutate1Start     VirtualizerStart
#define VirtualizerMutate2Start     VirtualizerStart
#define VirtualizerMutate3Start     VirtualizerStart

#define VIRTUALIZER1_START          VIRTUALIZER_START
#define VIRTUALIZER2_START          VIRTUALIZER_START
#define VIRTUALIZER3_START          VIRTUALIZER_START
#define VIRTUALIZER4_START          VIRTUALIZER_START
#define VIRTUALIZER5_START          VIRTUALIZER_START
#define VIRTUALIZER1_END            VIRTUALIZER_END
#define VIRTUALIZER2_END            VIRTUALIZER_END
#define VIRTUALIZER3_END            VIRTUALIZER_END
#define VIRTUALIZER4_END            VIRTUALIZER_END
#define VIRTUALIZER5_END            VIRTUALIZER_END
#define VIRTUALIZER_MUTATE1_START   VIRTUALIZER_START
#define VIRTUALIZER_MUTATE2_START   VIRTUALIZER_START
#define VIRTUALIZER_MUTATE3_START   VIRTUALIZER_START


// ***********************************************
// Definition of VirtualizerStart macro to keep
// compatibility with previous CV versions
// ***********************************************

 #ifdef __cplusplus
  extern "C" {
 #endif

 __declspec(dllimport) void __stdcall VirtualizerStart(void);
 __declspec(dllimport) void __stdcall VirtualizerEnd(void);

 #ifdef __cplusplus
 }
 #endif

#if defined(PLATFORM_X64) && !defined(CV_X64_INSERT_VIA_INLINE)

 #define VIRTUALIZER_START VirtualizerStart();
 #define VIRTUALIZER_END VirtualizerEnd();

 #define CV_CUSTOM_VMS_DEFINED

#endif

#if defined(PLATFORM_X32) && !defined(CV_X32_INSERT_VIA_INLINE)

 #define VIRTUALIZER_START VirtualizerStart();
 #define VIRTUALIZER_END VirtualizerEnd();

 #define CV_CUSTOM_VMS_DEFINED

#endif


// ***********************************************
// x32/x64 definition as inline assembly
// ***********************************************

#ifndef CV_CUSTOM_VMS_DEFINED

#ifdef __BORLANDC__
  #include "VirtualizerSDK_BorlandC_inline.h"
#endif

#ifdef __GNUC__
  #include "VirtualizerSDK_GNU_inline.h"
#endif

#ifdef __ICL
  #include "VirtualizerSDK_ICL_inline.h"
#endif

#ifdef __LCC__
  #include "VirtualizerSDK_LCC_inline.h"
#endif

#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
  #include "VirtualizerSDK_VC_inline.h"
#endif

#endif
