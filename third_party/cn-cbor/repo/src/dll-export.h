// Contents of DLLDefines.h
#ifndef _cn_cbor_DLLDEFINES_H_
#define _cn_cbor_DLLDEFINES_H_

/* Cmake will define cn_cbor_EXPORTS on Windows when it
configures to build a shared library. If you are going to use
another build system on windows or create the visual studio
projects by hand you need to define cn_cbor_EXPORTS when
building a DLL on windows.
*/
// We are using the Visual Studio Compiler and building Shared libraries

#if defined (_WIN32) 
#if defined(CN_CBOR_IS_DLL)
    #define  MYLIB_EXPORT __declspec(dllexport)
#else
    #define MYLIB_EXPORT
#endif /* cn_cbor_EXPORTS */
#else /* defined (_WIN32) */
 #define MYLIB_EXPORT
#endif

#endif /* _cn_cbor_DLLDEFINES_H_ */
