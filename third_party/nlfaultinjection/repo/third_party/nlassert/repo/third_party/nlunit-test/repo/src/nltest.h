/**
 *    Copyright 2012-2017 Nest Labs Inc. All Rights Reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

/**
 *    @file
 *      This file is a backward-compatibility header for existing code
 *      that is used to including <nltest.h> rather than <nlunit-test.h>.
 *
 */

#ifndef __NLTEST_H_INCLUDED__
#define __NLTEST_H_INCLUDED__

#warning "Inclusion of this file is deprecated. Please include nlunit-test.h."

#include <nlunit-test.h>

#endif /* __NLTEST_H_INCLUDED__ */
