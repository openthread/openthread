/********************************************************************************************************
 * @file	string.h
 *
 * @brief	This is the header file for B91
 *
 * @author	B.Y
 * @date	2019
 *
 * @par     Copyright (c) 2019, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *          All rights reserved.
 *          
 *          Redistribution and use in source and binary forms, with or without
 *          modification, are permitted provided that the following conditions are met:
 *          
 *              1. Redistributions of source code must retain the above copyright
 *              notice, this list of conditions and the following disclaimer.
 *          
 *              2. Unless for usage inside a TELINK integrated circuit, redistributions 
 *              in binary form must reproduce the above copyright notice, this list of 
 *              conditions and the following disclaimer in the documentation and/or other
 *              materials provided with the distribution.
 *          
 *              3. Neither the name of TELINK, nor the names of its contributors may be 
 *              used to endorse or promote products derived from this software without 
 *              specific prior written permission.
 *          
 *              4. This software, with or without modification, must only be used with a
 *              TELINK integrated circuit. All other usages are subject to written permission
 *              from TELINK and different commercial license may apply.
 *
 *              5. Licensee shall be solely responsible for any claim to the extent arising out of or 
 *              relating to such deletion(s), modification(s) or alteration(s).
 *         
 *          THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 *          ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *          WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *          DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDER BE LIABLE FOR ANY
 *          DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *          (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *          LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *          ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *          (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *          SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *         
 *******************************************************************************************************/
#pragma once

#ifndef NULL
#define NULL  0
#endif

void *  memmove(void * dest, const void * src, unsigned int n);
void *  memset(void * d, int c, unsigned int  n);
void *  memcpy(void * des_ptr, const void * src_ptr, unsigned int );

// do not return void*,  otherwise, we must use a variable to store the dest porinter, that is not performance 
void   	memcpy4(void * dest, const void * src, unsigned int);
void *	memchr(const void *_s, int _c, unsigned int _n);
int		memcmp(const void *_s1, const void *_s2, unsigned int _n);

char *	strcat(char *_s1, const char *_s2);
char *	strchr(const char *_s, int _c);
int		strcmp(const char *_s1, const char *_s2);
int		strcoll(const char *_s1, const char *_s2);
char *	strcpy(char *_s1, const char *_s2);
unsigned int	strcspn(const char *_s1, const char *_s2);
char *	strerror(int _errcode);
unsigned int	strlen(const char *_s);
char *	strncat(char *_s1, const char *_s2, unsigned int _n);
int		strncmp(const char *_s1, const char *_s2, unsigned int _n);
char *	strncpy(char *_s1, const char *_s2, unsigned int _n);
char *	strpbrk(const char *_s1, const char *_s2);
char *	strrchr(const char *_s, int _c);
unsigned int	strspn(const char *_s1, const char *_s2);
char *	strstr(const char *_s1, const char *_s2);
char *	strtok(char *_s1, const char *_s2);
unsigned int	strxfrm(char *_s1, const char *_s2, unsigned int _n);
char * 	strchr (register const char *s, int c);
void * 	memchr (register const void * src_void, int c, unsigned int length);
int ismemzero4(void *data, unsigned int len);
void * memset4(void * dest, int val, unsigned int len);
void zeromem4(void *data, unsigned int len);


