/*********************************************************************
*                    SEGGER Microcontroller GmbH                     *
*                        The Embedded Experts                        *
**********************************************************************
*                                                                    *
*            (c) 1995 - 2018 SEGGER Microcontroller GmbH             *
*                                                                    *
*       www.segger.com     Support: support@segger.com               *
*                                                                    *
**********************************************************************
*                                                                    *
*       SEGGER RTT * Real Time Transfer for embedded targets         *
*                                                                    *
**********************************************************************
*                                                                    *
* All rights reserved.                                               *
*                                                                    *
* SEGGER strongly recommends to not make any changes                 *
* to or modify the source code of this software in order to stay     *
* compatible with the RTT protocol and J-Link.                       *
*                                                                    *
* Redistribution and use in source and binary forms, with or         *
* without modification, are permitted provided that the following    *
* conditions are met:                                                *
*                                                                    *
* o Redistributions of source code must retain the above copyright   *
*   notice, this list of conditions and the following disclaimer.    *
*                                                                    *
* o Redistributions in binary form must reproduce the above          *
*   copyright notice, this list of conditions and the following      *
*   disclaimer in the documentation and/or other materials provided  *
*   with the distribution.                                           *
*                                                                    *
* o Neither the name of SEGGER Microcontroller GmbH                  *
*   nor the names of its contributors may be used to endorse or      *
*   promote products derived from this software without specific     *
*   prior written permission.                                        *
*                                                                    *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND             *
* CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,        *
* INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF           *
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE           *
* DISCLAIMED. IN NO EVENT SHALL SEGGER Microcontroller BE LIABLE FOR *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR           *
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT  *
* OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;    *
* OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF      *
* LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT          *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE  *
* USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH   *
* DAMAGE.                                                            *
*                                                                    *
**********************************************************************
---------------------------END-OF-HEADER------------------------------
File    : SEGGER_RTT.h
Purpose : Implementation of SEGGER real-time transfer which allows
          real-time communication on targets which support debugger 
          memory accesses while the CPU is running.
Revision: $Rev: 12826 $
----------------------------------------------------------------------
*/

#ifndef SEGGER_RTT_H
#define SEGGER_RTT_H

#ifdef SEGGER_RTT_CONFIG_H
#include SEGGER_RTT_CONFIG_H
#else
#include "SEGGER_RTT_Conf.h"
#endif

#ifndef SEGGER_RTT_ASM  // defined when SEGGER_RTT.h is included from assembly file
#include <stdlib.h>
#include <stdarg.h>

/*********************************************************************
*
*       Defines, fixed
*
**********************************************************************
*/

/*********************************************************************
*
*       Types
*
**********************************************************************
*/

//
// Description for a circular buffer (also called "ring buffer")
// which is used as up-buffer (T->H)
//
typedef struct {
  const     char*    sName;         // Optional name. Standard names so far are: "Terminal", "SysView", "J-Scope_t4i4"
            char*    pBuffer;       // Pointer to start of buffer
            unsigned SizeOfBuffer;  // Buffer size in bytes. Note that one byte is lost, as this implementation does not fill up the buffer in order to avoid the problem of being unable to distinguish between full and empty.
            unsigned WrOff;         // Position of next item to be written by either target.
  volatile  unsigned RdOff;         // Position of next item to be read by host. Must be volatile since it may be modified by host.
            unsigned Flags;         // Contains configuration flags
} SEGGER_RTT_BUFFER_UP;

//
// Description for a circular buffer (also called "ring buffer")
// which is used as down-buffer (H->T)
//
typedef struct {
  const     char*    sName;         // Optional name. Standard names so far are: "Terminal", "SysView", "J-Scope_t4i4"
            char*    pBuffer;       // Pointer to start of buffer
            unsigned SizeOfBuffer;  // Buffer size in bytes. Note that one byte is lost, as this implementation does not fill up the buffer in order to avoid the problem of being unable to distinguish between full and empty.
  volatile  unsigned WrOff;         // Position of next item to be written by host. Must be volatile since it may be modified by host.
            unsigned RdOff;         // Position of next item to be read by target (down-buffer).
            unsigned Flags;         // Contains configuration flags
} SEGGER_RTT_BUFFER_DOWN;

//
// RTT control block which describes the number of buffers available
// as well as the configuration for each buffer
//
//
typedef struct {
  char                    acID[16];                                 // Initialized to "SEGGER RTT"
  int                     MaxNumUpBuffers;                          // Initialized to SEGGER_RTT_MAX_NUM_UP_BUFFERS (type. 2)
  int                     MaxNumDownBuffers;                        // Initialized to SEGGER_RTT_MAX_NUM_DOWN_BUFFERS (type. 2)
  SEGGER_RTT_BUFFER_UP    aUp[SEGGER_RTT_MAX_NUM_UP_BUFFERS];       // Up buffers, transferring information up from target via debug probe to host
  SEGGER_RTT_BUFFER_DOWN  aDown[SEGGER_RTT_MAX_NUM_DOWN_BUFFERS];   // Down buffers, transferring information down from host via debug probe to target
} SEGGER_RTT_CB;

/*********************************************************************
*
*       Global data
*
**********************************************************************
*/
extern SEGGER_RTT_CB _SEGGER_RTT;

/*********************************************************************
*
*       RTT API functions
*
**********************************************************************
*/
#ifdef __cplusplus
  extern "C" {
#endif
int          SEGGER_RTT_AllocDownBuffer         (const char* sName, void* pBuffer, unsigned BufferSize, unsigned Flags);
int          SEGGER_RTT_AllocUpBuffer           (const char* sName, void* pBuffer, unsigned BufferSize, unsigned Flags);
int          SEGGER_RTT_ConfigUpBuffer          (unsigned BufferIndex, const char* sName, void* pBuffer, unsigned BufferSize, unsigned Flags);
int          SEGGER_RTT_ConfigDownBuffer        (unsigned BufferIndex, const char* sName, void* pBuffer, unsigned BufferSize, unsigned Flags);
int          SEGGER_RTT_GetKey                  (void);
unsigned     SEGGER_RTT_HasData                 (unsigned BufferIndex);
int          SEGGER_RTT_HasKey                  (void);
unsigned     SEGGER_RTT_HasDataUp               (unsigned BufferIndex);
void         SEGGER_RTT_Init                    (void);
unsigned     SEGGER_RTT_Read                    (unsigned BufferIndex,       void* pBuffer, unsigned BufferSize);
unsigned     SEGGER_RTT_ReadNoLock              (unsigned BufferIndex,       void* pData,   unsigned BufferSize);
int          SEGGER_RTT_SetNameDownBuffer       (unsigned BufferIndex, const char* sName);
int          SEGGER_RTT_SetNameUpBuffer         (unsigned BufferIndex, const char* sName);
int          SEGGER_RTT_SetFlagsDownBuffer      (unsigned BufferIndex, unsigned Flags);
int          SEGGER_RTT_SetFlagsUpBuffer        (unsigned BufferIndex, unsigned Flags);
int          SEGGER_RTT_WaitKey                 (void);
unsigned     SEGGER_RTT_Write                   (unsigned BufferIndex, const void* pBuffer, unsigned NumBytes);
unsigned     SEGGER_RTT_WriteNoLock             (unsigned BufferIndex, const void* pBuffer, unsigned NumBytes);
unsigned     SEGGER_RTT_WriteSkipNoLock         (unsigned BufferIndex, const void* pBuffer, unsigned NumBytes);
unsigned     SEGGER_RTT_WriteString             (unsigned BufferIndex, const char* s);
void         SEGGER_RTT_WriteWithOverwriteNoLock(unsigned BufferIndex, const void* pBuffer, unsigned NumBytes);
unsigned     SEGGER_RTT_PutChar                 (unsigned BufferIndex, char c);
unsigned     SEGGER_RTT_PutCharSkip             (unsigned BufferIndex, char c);
unsigned     SEGGER_RTT_PutCharSkipNoLock       (unsigned BufferIndex, char c);
//
// Function macro for performance optimization
//
#define      SEGGER_RTT_HASDATA(n)       (_SEGGER_RTT.aDown[n].WrOff - _SEGGER_RTT.aDown[n].RdOff)

/*********************************************************************
*
*       RTT "Terminal" API functions
*
**********************************************************************
*/
int     SEGGER_RTT_SetTerminal        (char TerminalId);
int     SEGGER_RTT_TerminalOut        (char TerminalId, const char* s);

/*********************************************************************
*
*       RTT printf functions (require SEGGER_RTT_printf.c)
*
**********************************************************************
*/
int SEGGER_RTT_printf(unsigned BufferIndex, const char * sFormat, ...);
int SEGGER_RTT_vprintf(unsigned BufferIndex, const char * sFormat, va_list * pParamList);

#ifdef __cplusplus
  }
#endif

#endif // SEGGER_RTT_ASM

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/

//
// Operating modes. Define behavior if buffer is full (not enough space for entire message)
//
#define SEGGER_RTT_MODE_NO_BLOCK_SKIP         (0)     // Skip. Do not block, output nothing. (Default)
#define SEGGER_RTT_MODE_NO_BLOCK_TRIM         (1)     // Trim: Do not block, output as much as fits.
#define SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL    (2)     // Block: Wait until there is space in the buffer.
#define SEGGER_RTT_MODE_MASK                  (3)

//
// Control sequences, based on ANSI.
// Can be used to control color, and clear the screen
//
#define RTT_CTRL_RESET                "\x1B[0m"         // Reset to default colors
#define RTT_CTRL_CLEAR                "\x1B[2J"         // Clear screen, reposition cursor to top left

#define RTT_CTRL_TEXT_BLACK           "\x1B[2;30m"
#define RTT_CTRL_TEXT_RED             "\x1B[2;31m"
#define RTT_CTRL_TEXT_GREEN           "\x1B[2;32m"
#define RTT_CTRL_TEXT_YELLOW          "\x1B[2;33m"
#define RTT_CTRL_TEXT_BLUE            "\x1B[2;34m"
#define RTT_CTRL_TEXT_MAGENTA         "\x1B[2;35m"
#define RTT_CTRL_TEXT_CYAN            "\x1B[2;36m"
#define RTT_CTRL_TEXT_WHITE           "\x1B[2;37m"

#define RTT_CTRL_TEXT_BRIGHT_BLACK    "\x1B[1;30m"
#define RTT_CTRL_TEXT_BRIGHT_RED      "\x1B[1;31m"
#define RTT_CTRL_TEXT_BRIGHT_GREEN    "\x1B[1;32m"
#define RTT_CTRL_TEXT_BRIGHT_YELLOW   "\x1B[1;33m"
#define RTT_CTRL_TEXT_BRIGHT_BLUE     "\x1B[1;34m"
#define RTT_CTRL_TEXT_BRIGHT_MAGENTA  "\x1B[1;35m"
#define RTT_CTRL_TEXT_BRIGHT_CYAN     "\x1B[1;36m"
#define RTT_CTRL_TEXT_BRIGHT_WHITE    "\x1B[1;37m"

#define RTT_CTRL_BG_BLACK             "\x1B[24;40m"
#define RTT_CTRL_BG_RED               "\x1B[24;41m"
#define RTT_CTRL_BG_GREEN             "\x1B[24;42m"
#define RTT_CTRL_BG_YELLOW            "\x1B[24;43m"
#define RTT_CTRL_BG_BLUE              "\x1B[24;44m"
#define RTT_CTRL_BG_MAGENTA           "\x1B[24;45m"
#define RTT_CTRL_BG_CYAN              "\x1B[24;46m"
#define RTT_CTRL_BG_WHITE             "\x1B[24;47m"

#define RTT_CTRL_BG_BRIGHT_BLACK      "\x1B[4;40m"
#define RTT_CTRL_BG_BRIGHT_RED        "\x1B[4;41m"
#define RTT_CTRL_BG_BRIGHT_GREEN      "\x1B[4;42m"
#define RTT_CTRL_BG_BRIGHT_YELLOW     "\x1B[4;43m"
#define RTT_CTRL_BG_BRIGHT_BLUE       "\x1B[4;44m"
#define RTT_CTRL_BG_BRIGHT_MAGENTA    "\x1B[4;45m"
#define RTT_CTRL_BG_BRIGHT_CYAN       "\x1B[4;46m"
#define RTT_CTRL_BG_BRIGHT_WHITE      "\x1B[4;47m"


#endif

/*************************** End of file ****************************/
