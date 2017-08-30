/*
 *   Copyright 2010-2016 Nest Labs Inc. All Rights Reserved.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */

/**
 *    @file
 *      This file defines macros and interfaces for performing both
 *      compile- and run-time assertion checking and run-time
 *      exception handling.
 *
 *      Where exception-handing is concerned, the format of the macros
 *      are inspired by those found in Mac OS Classic and, later, Mac
 *      OS X. These, in turn, were inspired by "Living In an
 *      Exceptional World" by Sean Parent (develop, The Apple
 *      Technical Journal, Issue 11, August/September 1992)
 *      <http://www.mactech.com/articles/develop/issue_11/Parent_final.html>
 *      for the methodology behind these error handling and assertion
 *      macros.
 *
 */

/**
 *  @mainpage notitle
 *
 *  @section introduction Introduction
 *
 *  This package defines macros and interfaces for performing both
 *  compile- and run-time assertion checking and run-time exception
 *  handling.
 *
 *  Where exception-handing is concerned, the format of the macros are
 *  inspired by those found in Mac OS Classic and, later, Mac OS
 *  X. These, in turn, were inspired by "Living In an Exceptional
 *  World" by Sean Parent (develop, The Apple Technical Journal, Issue
 *  11, August/September 1992). See:
 *
 *    http://www.mactech.com/articles/develop/issue_11/Parent_final.html
 *
 *  for the methodology behind these error handling and assertion
 *  macros.
 *
 *  @section overview Overview
 *
 *  The interfaces in this package come in two interface modalities:
 *
 *  <dl>
 *     <dt>[Run-time](@ref run-time)</dt>
 *         <dd>Interfaces that dynamically check a logical assertion and
 *             alter run-time execution on assertion firing.</dd>
 *     <dt>[Compile-time](@ref compile-time)</dt>
 *         <dd>Interfaces that statically check a logical assertion and
 *             terminate compile-time execution on assertion firing.</dd>
 *  </dl>
 *
 *  @subsection run-time Run-time
 *
 *  The [run-time modality interfaces](@ref run-time-interfaces) in
 *  this package come in three families:
 *
 *  <dl>
 *      <dt>[Assertion](@ref run-time-assertions)</dt>
 *          <dd>Similar to the traditional C Standard Library
 *          [assert()](http://pubs.opengroup.org/onlinepubs/009695399/functions/assert.html).</dd>
 *
 *      <dt>[Precondition](@ref preconditions)</dt>
 *          <dd>Designed to be placed at the head of an interface or
 *          method to check incoming parameters and return on
 *          assertion failure.</dd>
 *
 *      <dt>[Exception](@ref exceptions)</dt>
 *          <dd>Designed to jump to a local label on assertion failure
 *          to support the method of error and exception handling that
 *          advocates a single function or method return site and, by
 *          extension, consolidated points of exception and error
 *          handling as well as resource clean-up.</dd>
 *  </dl>
 *
 *  There are several styles of interfaces within each family and
 *  several potential variants within each style, all of which are
 *  summarized below and are described in detail in the following
 *  sections.
 *
 *  @subsection compile-time Compile-time
 *
 *  The [compile-time modality interfaces](@ref compile-time-interfaces)
 *  in this package are simpler and come in a single family with a
 *  couple of variants.
 *
 *  @section run-time-interfaces Run-time Interfaces
 *
 *  @subsection triggers Behavior Triggers
 *
 *  Unlike the traditional C Standard Library
 *  [assert()](http://pubs.opengroup.org/onlinepubs/009695399/functions/assert.html)
 *  or other assertion checking packages, this package offers the
 *  ability to enable and customize one or more of a few triggers that
 *  run when an assertion fires, including:
 *
 *     * [Abort](@ref abort-behavior)
 *     * [Backtrace](@ref backtrace-behavior)
 *     * [Log](@ref log-behavior)
 *     * [Trap](@ref trap-behavior)
 *
 *  @subsubsection abort-behavior Abort
 *
 *  The abort behavior trigger allows, on an assertion firing, to
 *  execute a trigger that terminates overall program or system
 *  execution.
 *
 *  Note, however, the abort behavior trigger is only available in
 *  some styles of the [assertion-family](@ref run-time-assertions) of
 *  interfaces.
 *
 *  Please see #NL_ASSERT_ABORT() and @ref customization for more
 *  information about this behavior trigger.
 *
 *  @subsubsection backtrace-behavior Backtrace
 *
 *  The backtrace behavior trigger allows, on an assertion firing, to
 *  execute a trigger that generates a stack backtrace.
 *
 *  This style of assertion is available, when configured, on all
 *  interface families.
 *
 *  Please see #NL_ASSERT_BACKTRACE() and @ref customization for more
 *  information about this behavior trigger.
 *
 *  @subsubsection log-behavior Log
 *
 *  The log behavior trigger allows, on an assertion firing, to
 *  execute a trigger that logs a message summarizing the assertion
 *  that fired.
 *
 *  This style of assertion is available, when configured, on all
 *  interface families.
 *
 *  Please see #NL_ASSERT_LOG() and @ref customization for more
 *  information about this behavior trigger.
 *
 *  @subsubsection trap-behavior Trap
 *
 *  The log behavior trigger allows, on an assertion firing, to
 *  execute a trigger that generates a debugger trap or exception.
 *
 *  This style of assertion is available, when configured, on all
 *  interface families.
 *
 *  Please see #NL_ASSERT_TRAP() and @ref customization for more
 *  information about this behavior trigger.
 *
 *  @subsection run-time-assertions Assertion Interfaces
 *
 *  The assertion interfaces are similar to the traditional C
 *  Standard Library
 *  [assert()](http://pubs.opengroup.org/onlinepubs/009695399/functions/assert.html).
 *
 *  These interfaces include the following styles:
 *
 *    * [Assert](@ref run-time-assert)
 *    * [Abort](@ref run-time-abort)
 *    * [Check](@ref run-time-check)
 *    * [Verify](@ref run-time-verify)
 *
 *  The following table summarizes the relationship and features among
 *  the styles:

 *  <table>
 *      <tr>
 *          <th rowspan="2">Style</th>
 *          <th colspan="4">Behaviors</th>
 *          <th colspan="2">Availability</th>
 *      </tr>
 *      <tr>
 *          <th>Abort</th>
 *          <th>Backtrace</th>
 *          <th>Log</th>
 *          <th>Trap</th>
 *          <th>Non-production</th>
 *          <th>Production</th>
 *      </tr>
 *      <tr>
 *          <td>Assert</td>
 *          <td align="center">Non-production</td>
 *          <td align="center">Non-production</td>
 *          <td align="center">Non-production</td>
 *          <td align="center"></td>
 *          <td align="center">X</td>
 *          <td align="center"></td>
 *      </tr>
 *      <tr>
 *          <td>Abort</td>
 *          <td align="center">X</td>
 *          <td align="center">X</td>
 *          <td align="center">X</td>
 *          <td align="center"></td>
 *          <td align="center">X</td>
 *          <td align="center">X</td>
 *      </tr>
 *      <tr>
 *          <td>Check</td>
 *          <td align="center"></td>
 *          <td align="center">Non-production</td>
 *          <td align="center">Non-production</td>
 *          <td align="center">Non-production</td>
 *          <td align="center">X</td>
 *          <td align="center"></td>
 *      </tr>
 *      <tr>
 *          <td>Verify</td>
 *          <td align="center"></td>
 *          <td align="center">X</td>
 *          <td align="center">X</td>
 *          <td align="center">Non-production</td>
 *          <td align="center">X</td>
 *          <td align="center">X</td>
 *      </tr>
 *  </table>
 *
 *  @note The above described behaviors are only in effect when
 *        #NL_ASSERT_USE_FLAGS_DEFAULT to 1. See @ref customization
 *        for more information on configuring and customizing the
 *        trigger behaviors.
 *
 *  @subsubsection run-time-assert Assert
 *
 *  These assertions are identical to the traditional C Standard
 *  Library-style assertion (see
 *  [assert()](http://pubs.opengroup.org/onlinepubs/009695399/functions/assert.html))
 *  except that side-effects, if any, in the asserted expression will be
 *  produced even when the assertion is made inactive, as in production build
 *  configurations, by setting #NL_ASSERT_PRODUCTION to true.
 *
 *  Like the C Standard Library assertion, a trigger of this style of
 *  assertion interface will invoke #NL_ASSERT_ABORT().
 *
 *  @subsubsection run-time-abort Abort
 *
 *  These assertions are identical to the @ref run-time-assert style
 *  assertions; however, they are active in __both__ non-production
 *  __and__ production build configurations.
 *
 *  A trigger of this style of assertion interface will invoke #NL_ASSERT_ABORT().
 *
 *  @subsubsection run-time-check Check
 *
 *  These assertions are similar to the @ref run-time-assert style; however,
 *  this style __does not__ abort. Normal program flow and execution
 *  will continue past this assertion.
 *
 *  Side-effects, if any, in the asserted expression will be produced even when
 *  the assertion is made inactive, as in production build configurations, by
 *  setting #NL_ASSERT_PRODUCTION to true.
 *
 *  @subsubsection run-time-verify Verify
 *
 *  These assertions are similar to the @ref run-time-abort style; however,
 *  this style __does not__ abort. Normal program flow and execution
 *  will continue past this assertion.
 *
 *  These are active in __both__ non-production __and__ production
 *  build configurations.
 *
 *  @subsection preconditions Precondition Interfaces
 *
 *  These assertions are designed to be placed at the head of an
 *  interface or method to check incoming parameters.
 *
 *  These assertions implicitly return, either void or a specified
 *  value for non-void interfaces or methods.
 *
 *  @note This family of interfaces may be in violation of your site's
 *  or project's coding style and best practices by virtue of its
 *  implicit return. If so, please consider using the
 *  [exception-style](@ref exceptions) interfaces instead.
 *
 *  The following table summarizes the features for this family of interfaces:
 *
 *  <table>
 *      <tr>
 *          <th rowspan="2">Style</th>
 *          <th colspan="4">Behaviors</th>
 *          <th colspan="2">Availability</th>
 *      </tr>
 *      <tr>
 *          <th>Abort</th>
 *          <th>Backtrace</th>
 *          <th>Log</th>
 *          <th>Trap</th>
 *          <th>Non-production</th>
 *          <th>Production</th>
 *      </tr>
 *      <tr>
 *          <td>Precondition</td>
 *          <td align="center"></td>
 *          <td align="center">X</td>
 *          <td align="center">X</td>
 *          <td align="center"></td>
 *          <td align="center">X</td>
 *          <td align="center">X</td>
 *      </tr>
 *  </table>
 *
 *  @note The above described behaviors are only in effect when
 *        #NL_ASSERT_USE_FLAGS_DEFAULT to 1. See @ref customization
 *        for more information on configuring and customizing the
 *        trigger behaviors.
 *
 *  @subsection exceptions Exception Interfaces
 *
 *  This family of interfaces is designed to support the method of
 *  error and exception handling that advocates a single function or
 *  method return site and, by extension, consolidated points of
 *  exception and error handling as well as resource clean-up.
 *
 *  A general example usage of this family of interfaces is:
 *
 *  @code
 *  int Bar(uint8_t **aBuffer, const Foo *aParameter)
 *  {
 *      const size_t size   = 1024;
 *      int          retval = 0;
 *
 *      nlREQUIRE(aBuffer != NULL, exit, retval = -EINVAL);
 *
 *      *aBuffer = (uint8_t *)malloc(size);
 *      nlREQUIRE(*aBuffer != NULL, exit, retval = -ENOMEM);
 *
 *      memset(*aBuffer, 0, size);
 *
 *  exit:
 *      return retval;
 *  }
 *  @endcode
 *
 *  As shown in the example, this family checks for the specified
 *  condition, which is expected to commonly be true, and branches to
 *  the specified label if the condition is false.
 *
 *    * [Expect](@ref expect)
 *    * [Desire](@ref desire)
 *    * [Require](@ref require)
 *
 *  This family of interfaces are all identical across the styles and
 *  all styles support an identical set of variants. The only
 *  difference among them is the default configured trigger action
 *  behavior, as summarized in the table below:
 *
 *  <table>
 *      <tr>
 *          <th rowspan="2">Style</th>
 *          <th colspan="3">Behaviors</th>
 *          <th colspan="2">Availability</th>
 *      </tr>
 *      <tr>
 *          <th>Backtrace</th>
 *          <th>Log</th>
 *          <th>Trap</th>
 *          <th>Non-production</th>
 *          <th>Production</th>
 *      </tr>
 *      <tr>
 *          <td>Expect</td>
 *          <td></td>
 *          <td></td>
 *          <td></td>
 *          <td align="center">X</td>
 *          <td align="center">X</td>
 *      </tr>
 *      <tr>
 *          <td>Desire</td>
 *          <td></td>
 *          <td align="center">Non-production</td>
 *          <td></td>
 *          <td align="center">X</td>
 *          <td align="center">X</td>
 *      </tr>
 *      <tr>
 *          <td>Require</td>
 *          <td align="center">X</td>
 *          <td align="center">X</td>
 *          <td align="center">Non-production</td>
 *          <td align="center">X</td>
 *          <td align="center">X</td>
 *      </tr>
 *  </table>
 *
 *  @note The above described behaviors are only in effect when
 *        #NL_ASSERT_USE_FLAGS_DEFAULT to 1. See @ref customization
 *        for more information on configuring and customizing the
 *        trigger behaviors.
 *
 *  @subsubsection expect Expect
 *
 *  These assertions are designed to be placed anywhere an exceptional
 *  condition might occur where handling needs to locally jump to
 *  error-handling code.
 *
 *  These checks are always present and do nothing beyond branching to
 *  the named exception label. Consequently, they are simply mnemonics
 *  or syntactic sugar.
 *
 *  This style of check should be used where either true or false
 *  evaluation of the assertion expression are equally likely since
 *  there is no default configured trigger behavior.
 *
 *  __Anticipated Assertion Firing Frequency:__ Frequent
 *
 *  @subsubsection desire Desire
 *
 *  These are identical to the @ref expect style checks; except for
 *  their non-production and production configured behavior.
 *
 *  This style of check should be used where false evaluation of the
 *  assertion expression is expected to be occasional.
 *
 *  __Anticipated Assertion Firing Frequency:__ Occasional
 *
 *  @subsubsection require Require
 *
 *  These are identical to the @ref expect style checks; except for
 *  their non-production and production configured behavior.
 *
 *  This style of check should be used where false evaluation of the
 *  assertion expression is expected to be rare.
 *
 *  __Anticipated Assertion Firing Frequency:__ Rare
 *
 * @subsection variants Variants
 *
 * The three families of run-time interface are available in one of several
 * variants, as summarized below. <em>&lt;STYLE&gt;</em> may be replaced with one of (see [Run-time Availability](@ref run-time-availability) for details):
 *
 *   * ASSERT
 *   * ABORT
 *   * CHECK
 *   * VERIFY
 *   * PRECONDITION
 *   * EXPECT
 *   * DESIRE
 *   * REQUIRE
 *
 * to form an actual interface name.
 *
 * | Interface Variant                              | Description                                                                                                  |
 * |:---------------------------------------------- |:------------------------------------------------------------------------------------------------------------ |
 * | nl<em>&lt;STYLE&gt;</em>                       | Base variant; execute the check.                                                                             |
 * | nl<em>&lt;STYLE&gt;</em>\_ACTION               | Execute the base check and execute the action if the check fails.                                            |
 * | nl<em>&lt;STYLE&gt;</em>\_PRINT                | Execute the base check and print the descriptive string if the check fails.                                  |
 * | nl<em>&lt;STYLE&gt;</em>\_ACTION_PRINT         | Execute the base check and both execute the action and print the descriptive string if the check fails.      |
 * | nl<em>&lt;STYLE&gt;</em>\_SUCCESS              | Adds a check against zero (0) as the logical condition to assert.                                            |
 * | nl<em>&lt;STYLE&gt;</em>\_SUCCESS_ACTION       | Execute the success check and execute the action if the check fails.                                         |
 * | nl<em>&lt;STYLE&gt;</em>\_SUCCESS_PRINT        | Execute the success check and print the descriptive string if the check fails.                               |
 * | nl<em>&lt;STYLE&gt;</em>\_SUCCESS_ACTION_PRINT | Execute the success check and both execute the action and print the descriptive string if the check fails.   |
 * | nlN<em>&lt;STYLE&gt;</em>                      | Inverts the logical sense of the base check; execute the check.                                              |
 * | nlN<em>&lt;STYLE&gt;</em>\_ACTION              | Execute the inversion check and execute the action if the check fails.                                       |
 * | nlN<em>&lt;STYLE&gt;</em>\_PRINT               | Execute the inversion check and print the descriptive string if the check fails.                             |
 * | nlN<em>&lt;STYLE&gt;</em>\_ACTION_PRINT        | Execute the inversion check and both execute the action and print the descriptive string if the check fails. |
 *
 * @section run-time-availability Run-time Availability
 *
 * The following table indicates the availability of the run-time interface
 * variants for each style.
 *
 * <table>
 *     <tr>
 *         <th rowspan="2">Interface Variant</th>
 *         <th colspan="8">Style</th>
 *     </tr>
 *     <tr>
 *         <th>Assert</th>
 *         <th>Abort</th>
 *         <th>Check</th>
 *         <th>Verify</th>
 *         <th>Precondition</th>
 *         <th>Expect</th>
 *         <th>Desire</th>
 *         <th>Require</th>
 *     </tr>
 *     <tr>
 *         <td>nl<em>&lt;STYLE&gt;</em></td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *     </tr>
 *     <tr>
 *         <td>nl<em>&lt;STYLE&gt;</em>\_ACTION</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *     </tr>
 *     <tr>
 *         <td>nl<em>&lt;STYLE&gt;</em>\_PRINT</td>
 *         <td align="center"></td>
 *         <td align="center"></td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *     </tr>
 *     <tr>
 *         <td>nl<em>&lt;STYLE&gt;</em>\_ACTION_PRINT</td>
 *         <td align="center"></td>
 *         <td align="center"></td>
 *         <td align="center"></td>
 *         <td align="center"></td>
 *         <td align="center"></td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *     </tr>
 *     <tr>
 *         <td>nl<em>&lt;STYLE&gt;</em>\_SUCCESS</td>
 *         <td align="center"></td>
 *         <td align="center"></td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *     </tr>
 *     <tr>
 *         <td>nl<em>&lt;STYLE&gt;</em>\_SUCCESS_ACTION</td>
 *         <td align="center"></td>
 *         <td align="center"></td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *     </tr>
 *     <tr>
 *         <td>nl<em>&lt;STYLE&gt;</em>\_SUCCESS_PRINT</td>
 *         <td align="center"></td>
 *         <td align="center"></td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *     </tr>
 *     <tr>
 *         <td>nl<em>&lt;STYLE&gt;</em>\_SUCCESS_ACTION_PRINT</td>
 *         <td align="center"></td>
 *         <td align="center"></td>
 *         <td align="center"></td>
 *         <td align="center"></td>
 *         <td align="center"></td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *     </tr>
 *     <tr>
 *         <td>nlN<em>&lt;STYLE&gt;</em></td>
 *         <td align="center"></td>
 *         <td align="center"></td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *     </tr>
 *     <tr>
 *         <td>nlN<em>&lt;STYLE&gt;</em>\_ACTION</td>
 *         <td align="center"></td>
 *         <td align="center"></td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *     </tr>
 *     <tr>
 *         <td>nlN<em>&lt;STYLE&gt;</em>\_PRINT</td>
 *         <td align="center"></td>
 *         <td align="center"></td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *     </tr>
 *     <tr>
 *         <td>nlN<em>&lt;STYLE&gt;</em>\_ACTION_PRINT</td>
 *         <td align="center"></td>
 *         <td align="center"></td>
 *         <td align="center"></td>
 *         <td align="center"></td>
 *         <td align="center"></td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *     </tr>
 * </table>
 *
 * @section customization Customization
 *
 * The Nest Labs Assertion library is designed "out of the box" to
 * provide a high degree of utility. However, there are a number of
 * aspects that can be tuned on a per-module, -project, or -site basis
 * to customize the appearance and behavior through user-definable
 * attributes via the C preprocessor.
 *
 * @subsection userattrs User-definable Attributes
 *
 * The following attributes may be defined by the user before
 * the nlassert.h header is included by the preprocessor, overriding
 * the default behavior:
 *
 *   * #NL_ASSERT_PRODUCTION
 *   * #NL_ASSERT_PREFIX_STRING
 *   * #NL_ASSERT_COMPONENT_STRING
 *   * #NL_ASSERT_FILE
 *   * #NL_ASSERT_ABORT
 *   * #NL_ASSERT_BACKTRACE
 *   * #NL_ASSERT_LOG
 *   * #NL_ASSERT_TRAP
 *
 * Without any customization, all of the interface styles are set to
 * use __no__ trigger behaviors (i.e. #NL_ASSERT_FLAG_NONE). However, a
 * set of default trigger behaviors (as documented in the tables
 * above) may be enabled by setting #NL_ASSERT_USE_FLAGS_DEFAULT to 1
 * before nlassert.h is included by the preprocessor.
 *
 * Otherwise, the following attributes may be overridden, customizing
 * the trigger behavior of each assertion interface family for both
 * non-production and production configurations:
 *
 *   * #NL_ASSERT_EXPECT_FLAGS
 *
 *   * #NL_ASSERT_CHECK_NONPRODUCTION_FLAGS
 *   * #NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS
 *   * #NL_ASSERT_ASSERT_NONPRODUCTION_FLAGS
 *   * #NL_ASSERT_ABORT_NONPRODUCTION_FLAGS
 *   * #NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS
 *   * #NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS
 *   * #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS
 *
 *   * #NL_ASSERT_VERIFY_PRODUCTION_FLAGS
 *   * #NL_ASSERT_ABORT_PRODUCTION_FLAGS
 *   * #NL_ASSERT_DESIRE_PRODUCTION_FLAGS
 *   * #NL_ASSERT_REQUIRE_PRODUCTION_FLAGS
 *   * #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS
 *
 *  @section compile-time-interfaces Compile-time Interfaces
 *
 *  The compile-time interfaces are a limited subset of their run-time
 *  counterparts. Rather than altering run-time execution on assertion
 *  firing against a dynamically-checked run-time condition, these
 *  interfaces terminate compilation against a statically-checked
 *  compile-time condition.
 *
 *  @subsection compile-time-assertions Assertion Interfaces
 *
 *  These interfaces have only one style:
 *
 *    * [Assert](@ref compile-time-assert)
 *
 *  <table>
 *      <tr>
 *          <th rowspan="2">Style</th>
 *          <th colspan="2">Availability</th>
 *      </tr>
 *      <tr>
 *          <th>Non-production</th>
 *          <th>Production</th>
 *      </tr>
 *      <tr>
 *          <td>Assert</td>
 *          <td align="center">X</td>
 *          <td align="center">X</td>
 *      </tr>
 *  </table>
 *
 *  @subsubsection compile-time-assert Assert
 *
 *  These assertions are active in __both__ non-production
 *  __and__ production build configurations. This behavior
 *  is unconventional with respect to the [run-time assert-style assertions](@ref run-time-assert)
 *  but very conventional with respect to the similar
 *  C11/C++11 static-assertion standards.
 *
 * @subsection compile-time-variants Variants
 *
 * The compile-time interfaces are available in two variants:
 *
 * | Interface Variant                              | Description                                                                                                  |
 * |:---------------------------------------------- |:------------------------------------------------------------------------------------------------------------ |
 * | nlSTATIC\_ASSERT                               | Base variant; execute the check.                                                                             |
 * | nlSTATIC\_ASSERT\_PRINT                        | Execute the base check with a descriptive string. Note, this string is not actually emitted in any meaningful way. It serves to simply comment or annotate the assertion and to provide interface parallelism with the run-time assertion interfaces.                                  |
 *
 * @section compile-time-availability Compile-time Availability
 *
 * The following table indicates the availability of the compile-time interface
 * variants.
 *
 * <table>
 *     <tr>
 *         <th rowspan="2">Interface Variant</th>
 *         <th colspan="2">Availability</th>
 *     </tr>
 *     <tr>
 *         <th>Non-Production</th>
 *         <th>Production</th>
 *     </tr>
 *     <tr>
 *         <td>nlSTATIC\_ASSERT</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *     </tr>
 *     <tr>
 *         <td>nlSTATIC\_ASSERT\_PRINT</td>
 *         <td align="center">X</td>
 *         <td align="center">X</td>
 *     </tr>
 * </table>
 *
 * @section compatibility Standard C Library Compatibility
 *
 * This package also provides an ISO/IEC 9899:1999-, C89-, and
 * C99-compatible Standard C Library header (via assert.h) and
 * assertion interface definition (assert()), implemented atop Nest
 * Labs assertion checking and runtime exception interfaces such that
 * consistent platform and system capabilities, behavior, and output
 * may be implemented and enforced across the two interfaces.
 *
 * Systems wishing to use this compatibility header and interface in
 * lieu of their Standard C Library header of the same name should
 * ensure that their toolchain is configured to either ignore or
 * deprioritize standard search paths while placing the directory this
 * header is contained in among the preferred header search paths.
 *
 */

#ifndef NLCORE_NLASSERT_H
#define NLCORE_NLASSERT_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

/**
 *  @name Behavioral Control Flags
 *
 *  @brief
 *    These flags are used to influence the behavior of the various
 *    classes and styles of assertion macros.
 *
 *  @{
 *
 */

/**
 *  @def NL_ASSERT_FLAG_NONE
 *
 *  @brief
 *    Perform no actions when an assertion expression evaluates to
 *    false.
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 */
#define NL_ASSERT_FLAG_NONE             0x00000000

/**
 *  @def NL_ASSERT_FLAG_BACKTRACE
 *
 *  @brief
 *    Invoke #NL_ASSERT_BACKTRACE() when an assertion expression
 *    evaluates to false.
 *
 *  @note For the *_ACTION*-style assertion variants,
 *        #NL_ASSERT_BACKTRACE() is a pre-action trigger and will run
 *        before the specified action.
 *
 *  @sa #NL_ASSERT_FLAG_NONE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 *  @sa #NL_ASSERT_BACKTRACE
 *
 */
#define NL_ASSERT_FLAG_BACKTRACE        0x00000001

/**
 *  @def NL_ASSERT_FLAG_LOG
 *
 *  @brief
 *    Invoke #NL_ASSERT_LOG() when an assertion expression evaluates
 *    to false.
 *
 *  @note For the *_ACTION*-style assertion variants,
 *        #NL_ASSERT_LOG() is a pre-action trigger and will run
 *        before the specified action.
 *
 *  @sa #NL_ASSERT_FLAG_NONE
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 *  @sa #NL_ASSERT_LOG
 *
 */
#define NL_ASSERT_FLAG_LOG              0x00000002

/**
 *  @def NL_ASSERT_FLAG_TRAP
 *
 *  @brief
 *    Invoke #NL_ASSERT_TRAP() when an assertion expression evaluates
 *    to false.
 *
 *  @note For the *_ACTION*-style assertion variants,
 *        #NL_ASSERT_TRAP() is a post-action trigger and will run
 *        after the specified action.
 *
 *  @sa #NL_ASSERT_FLAG_NONE
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *
 *  @sa #NL_ASSERT_TRAP
 *
 */
#define NL_ASSERT_FLAG_TRAP             0x00000004

/**
 *  @}
 *
 */

/**
 *  @def NL_ASSERT_USE_FLAGS_DEFAULT
 *
 *  @brief
 *    Enable (1) or disable (0) use of the default trigger behavior flags.
 *
 *  This enables or disables the use of the default trigger behavior
 *  flags as specified by:
 *
 *   * #NL_ASSERT_EXPECT_FLAGS_DEFAULT
 *
 *   * #NL_ASSERT_CHECK_NONPRODUCTION_FLAGS_DEFAULT
 *   * #NL_ASSERT_VERIFY_NONPRODUCTION_FLAGS_DEFAULT
 *   * #NL_ASSERT_ASSERT_NONPRODUCTION_FLAGS_DEFAULT
 *   * #NL_ASSERT_ABORT_NONPRODUCTION_FLAGS_DEFAULT
 *   * #NL_ASSERT_DESIRE_NONPRODUCTION_FLAGS_DEFAULT
 *   * #NL_ASSERT_REQUIRE_NONPRODUCTION_FLAGS_DEFAULT
 *   * #NL_ASSERT_PRECONDITION_NONPRODUCTION_FLAGS_DEFAULT
 *
 *   * #NL_ASSERT_VERIFY_PRODUCTION_FLAGS_DEFAULT
 *   * #NL_ASSERT_ABORT_PRODUCTION_FLAGS_DEFAULT
 *   * #NL_ASSERT_DESIRE_PRODUCTION_FLAGS_DEFAULT
 *   * #NL_ASSERT_REQUIRE_PRODUCTION_FLAGS_DEFAULT
 *   * #NL_ASSERT_PRECONDITION_PRODUCTION_FLAGS_DEFAULT
 *
 *  Setting this to 1, effectively does the following:
 *
 *  @code
 *  #define NL_ASSERT_<STYLE>_<CONFIGURATION>_FLAGS NL_ASSERT_<STYLE>_<CONFIGURATION>_FLAGS_DEFAULT
 *  @endcode
 *
 *  for each assertion interface <em>&lt;STYLE&gt;</em> for both
 *  non-production and production (see #NL_ASSERT_PRODUCTION) <em>&lt;CONFIGURATION&gt;</em> .
 *
 */
#if !defined(NL_ASSERT_USE_FLAGS_DEFAULT)
#define NL_ASSERT_USE_FLAGS_DEFAULT     0
#endif /* !defined(NL_ASSERT_USE_FLAGS_DEFAULT) */

/**
 *  @def NL_ASSERT_PRODUCTION
 *
 *  @brief
 *    Enable (1) or disable (0) when production (or non-production)
 *    assertion behavior is desired.
 *
 *  When production behavior is asserted, a number of interface
 *  families are elided entirely and for others, the default behavior
 *  changes (default: ((defined(NDEBUG) && NDEBUG) || (defined(DEBUG)
 *  && !DEBUG) || 1)).
 *
 */
#if !defined(NL_ASSERT_PRODUCTION)
#if defined(NDEBUG)
#define NL_ASSERT_PRODUCTION            NDEBUG
#elif defined(DEBUG)
#define NL_ASSERT_PRODUCTION            !DEBUG
#else
#define NL_ASSERT_PRODUCTION            1
#endif /* defined(NDEBUG) */
#endif /* !defined(NL_ASSERT_PRODUCTION) */

/**
 *  @name Log Output Definitions
 *
 *  @brief
 *    These definitions control how the output of assertion log
 *    messages appear, when so configured, on assertion expression
 *    failure evaluation.
 *
 *  @{
 *
 */

/**
 *  @def NL_ASSERT_PREFIX_STRING
 *
 *  @brief
 *    This is the string printed at the beginning of the assertion
 *    printed (default: 'ASSERT: ').
 *
 *    Developers may, but are generally not encouraged to, override
 *    this by defining #NL_ASSERT_PREFIX_STRING before nlassert.h is
 *    included by the preprocessor, as shown in the following example:
 *
 *  @code
 *    #define NL_ASSERT_PREFIX_STRING   "assertion: "
 *
 *    #include <nlassert.h>
 *  @endcode
 *
 *  @sa #NL_ASSERT_PREFIX_STRING
 */
#if !defined(NL_ASSERT_PREFIX_STRING)
#define NL_ASSERT_PREFIX_STRING         "ASSERT: "
#endif /* !defined(NL_ASSERT_PREFIX_STRING) */

/**
 *  @def NL_ASSERT_COMPONENT_STRING
 *
 *  @brief
 *    This is the string printed following the prefix string (see
 *    #NL_ASSERT_PREFIX_STRING) that indicates what module, program,
 *    application or subsystem the assertion occurred in (default:
 *    '').
 *
 *    Developers may override this by defining
 *    #NL_ASSERT_COMPONENT_STRING before nlassert.h is included by the
 *    preprocessor, as shown in the following example:
 *
 *  @code
 *    #define NL_ASSERT_COMPONENT_STRING   "nlbar"
 *
 *    #include <nlassert.h>
 *  @endcode
 *
 *  @sa #NL_ASSERT_PREFIX_STRING
 *
 */
#if !defined(NL_ASSERT_COMPONENT_STRING)
#define NL_ASSERT_COMPONENT_STRING      ""
#endif  /* !defined(NL_ASSERT_COMPONENT_STRING) */

/**
 *  @}
 *
 */

/**
 *  @def NL_ASSERT_ABORT()
 *
 *  @brief
 *    This macro is invoked when an nlASSERT- or nlABORT-style
 *    assertion expression evaluates to false.
 *
 *    By default, this is defined to the C Standard Library
 *    [abort()](http://pubs.opengroup.org/onlinepubs/009695399/functions/abort.html). When
 *    overridden, this should generally provide similar functionality,
 *    terminating the overall process or system execution.
 *
 *    Developers may override and customize this by defining
 *    #NL_ASSERT_ABORT() before nlassert.h is included by the
 *    preprocessor.
 *
 */
#if !defined(NL_ASSERT_ABORT)
#define NL_ASSERT_ABORT()               abort()
#endif /* !defined(NL_ASSERT_ABORT) */

/**
 *  @def NL_ASSERT_BACKTRACE()
 *
 *  @brief
 *    This macro is invoked when an assertion expression evaluates to
 *    false when the macro has been configured with / passed
 *    #NL_ASSERT_FLAG_BACKTRACE.
 *
 *  By default, this is defined to do nothing. However, when defined,
 *  this should generally generate a stack back trace when invoked.
 *
 *  Developers may override and customize this by defining
 *  #NL_ASSERT_BACKTRACE() before nlassert.h is included by the
 *  preprocessor.
 *
 *  @note For the *_ACTION*-style assertion variants,
 *        #NL_ASSERT_BACKTRACE() is a pre-action trigger and will run
 *        before the specified action.
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *
 */
#if !defined(NL_ASSERT_BACKTRACE)
#define NL_ASSERT_BACKTRACE()
#endif /* !defined(NL_ASSERT_BACKTRACE) */

/**
 *  @def NL_ASSERT_TRAP()
 *
 *  @brief
 *    This macro is invoked when an assertion expression evaluates to
 *    false when the macro has been configured with / passed
 *    #NL_ASSERT_FLAG_TRAP.
 *
 *  By default, this is defined to do nothing. However, when defined,
 *  this should generally generate a debug trap or breakpoint such that
 *  when the assertion expression evaluates to false an attached
 *  debugger will stop at the assertion point.
 *
 *  Developers may override and customize this by defining
 *  #NL_ASSERT_TRAP() before nlassert.h is included by the
 *  preprocessor, as shown in the following example:
 *
 *  @code
 *    #if defined(__i386__) || defined(__x86_64__)
 *    #define DEBUGGER_TRAP    __asm__ __volatile__("int3")
 *    #elif defined(__arm__)
 *    #define DEBUGGER_TRAP    __asm__ __volatile__("bkpt")
 *    #endif
 *
 *    #define NL_ASSERT_TRAP()                               \
 *        do                                                 \
 *        {                                                  \
 *            if (IsDebuggerAttached())                      \
 *            {                                              \
 *                DEBUGGER_TRAP;                             \
 *            }                                              \
 *        } while (0)
 *
 *    #include <nlassert.h>
 *  @endcode
 *
 *  @note For the *_ACTION*-style assertion variants,
 *        #NL_ASSERT_TRAP() is a post-action trigger and will run
 *        after the specified action.
 *
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 */
#if !defined(NL_ASSERT_TRAP)
#define NL_ASSERT_TRAP()
#endif /* !defined(NL_ASSERT_TRAP) */

/**
 *  @def NL_ASSERT_LOG_FORMAT_DEFAULT
 *
 *  @brief
 *    This is the NULL-terminated C string literal with C Standard
 *    Library-style format specifiers used by #NL_ASSERT_LOG_DEFAULT.
 *
 *  This may be used by overrides to #NL_ASSERT_LOG that want to use a
 *  consistent output formatting.
 *
 *  @sa #NL_ASSERT_LOG_DEFAULT
 *
 */
#define NL_ASSERT_LOG_FORMAT_DEFAULT                           "%s%s%s%s, %s%sfile: %s, line: %d\n"

/**
 *  @def NL_ASSERT_LOG_DEFAULT(aPrefix, aName, aCondition, aLabel, aFile, aLine, aMessage)
 *
 *  @brief
 *    This macro conforms to #NL_ASSERT_LOG and may be assigned to
 *    #NL_ASSERT_LOG, as shown in the following example, to display an
 *    assertion message when the assertion triggers, via the C
 *    Standard I/O Library error stream.
 *
 *  @code
 *    #define NL_ASSERT_LOG(aPrefix, aName, aCondition, aLabel, aFile, aLine, aMessage) \
 *        NL_ASSERT_LOG_DEFAULT(aPrefix, aName, aCondition, aLabel, aFile, aLine, aMessage)
 *
 *    #include <nlassert.h>
 *  @endcode
 *
 *  @param[in]  aPrefix     A pointer to a NULL-terminated C string printed
 *                          at the beginning of the logged assertion
 *                          message. Typically this is and should be
 *                          #NL_ASSERT_PREFIX_STRING.
 *  @param[in]  aName       A pointer to a NULL-terminated C string printed
 *                          following @p aPrefix that indicates what
 *                          module, program, application or subsystem
 *                          the assertion occurred in Typically this
 *                          is and should be
 *                          #NL_ASSERT_COMPONENT_STRING.
 *  @param[in]  aCondition  A pointer to a NULL-terminated C string indicating
 *                          the expression that evaluated to false in
 *                          the assertion. Typically this is a
 *                          stringified version of the actual
 *                          assertion expression.
 *  @param[in]  aLabel      An optional pointer to a NULL-terminated C string
 *                          indicating, for exception-style
 *                          assertions, the label that will be
 *                          branched to when the assertion expression
 *                          evaluates to false.
 *  @param[in]  aFile       A pointer to a NULL-terminated C string indicating
 *                          the file in which the exception
 *                          occurred. Typically this is and should be
 *                          \_\_FILE\_\_ from the C preprocessor or
 *                          #NL_ASSERT_FILE.
 *  @param[in]  aLine       The line number in @p aFile on which the assertion
 *                          expression evaluated to false. Typically
 *                          this is and should be \_\_LINE\_\_ from the C
 *                          preprocessor.
 *  @param[in]  aMessage    An optional pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_LOG
 *
 *  @sa #NL_ASSERT_FLAG_LOG
 *
 *  @sa #NL_ASSERT_PREFIX_STRING
 *  @sa #NL_ASSERT_COMPONENT_STRING
 *  @sa #NL_ASSERT_LOG_FORMAT_DEFAULT
 *
 *  @sa #NL_ASSERT_FILE
 *
 */
#define NL_ASSERT_LOG_DEFAULT(aPrefix, aName, aCondition, aLabel, aFile, aLine, aMessage) \
    do                                                                                    \
    {                                                                                     \
        fprintf(stderr,                                                                   \
                NL_ASSERT_LOG_FORMAT_DEFAULT,                                             \
                aPrefix,                                                                  \
                (((aName) == 0) || (*(aName) == '\0')) ? "" : aName,                      \
                (((aName) == 0) || (*(aName) == '\0')) ? "" : ": ",                       \
                aCondition,                                                               \
                ((aMessage == 0) ? "" : aMessage),                                        \
                ((aMessage == 0) ? "" : ", "),                                            \
                aFile,                                                                    \
                aLine);                                                                   \
    } while (0)

/**
 *  @def NL_ASSERT_LOG(aPrefix, aName, aCondition, aLabel, aFile, aLine, aMessage)
 *
 *  @brief
 *    This macro is invoked when an assertion expression evaluates to
 *    false when the macro has been configured with / passed
 *    #NL_ASSERT_FLAG_LOG.
 *
 *  This is intended to display a message, to an appropriate log stream, informing the developer where the assertion was and what expression failed, similar to:
 *
 *      "ASSERT: MyComponent: aPointer != NULL, bad pointer, file: foo.c, line: 453"
 *
 *  Developers may override and customize this by defining
 *  #NL_ASSERT_LOG() before nlassert.h is included by the
 *  preprocessor.
 *
 *  @note For the *_ACTION*-style assertion variants, #NL_ASSERT_LOG()
 *        is a pre-action trigger and will run before the specified action.
 *
 *  @param[in]  aPrefix     A pointer to a NULL-terminated C string printed
 *                          at the beginning of the logged assertion
 *                          message. Typically this is and should be
 *                          #NL_ASSERT_PREFIX_STRING.
 *  @param[in]  aName       A pointer to a NULL-terminated C string printed
 *                          following @p aPrefix that indicates what
 *                          module, program, application or subsystem
 *                          the assertion occurred in Typically this
 *                          is and should be
 *                          #NL_ASSERT_COMPONENT_STRING.
 *  @param[in]  aCondition  A pointer to a NULL-terminated C string indicating
 *                          the expression that evaluated to false in
 *                          the assertion. Typically this is a
 *                          stringified version of the actual
 *                          assertion expression.
 *  @param[in]  aLabel      An optional pointer to a NULL-terminated C string
 *                          indicating, for exception-style
 *                          assertions, the label that will be
 *                          branched to when the assertion expression
 *                          evaluates to false.
 *  @param[in]  aFile       A pointer to a NULL-terminated C string indicating
 *                          the file in which the exception
 *                          occurred. Typically this is and should be
 *                          \_\_FILE\_\_ from the C preprocessor or
 *                          #NL_ASSERT_FILE.
 *  @param[in]  aLine       The line number in @p aFile on which the assertion
 *                          expression evaluated to false. Typically
 *                          this is and should be \_\_LINE\_\_ from the C
 *                          preprocessor.
 *  @param[in]  aMessage    An optional pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_LOG_DEFAULT
 *
 *  @sa #NL_ASSERT_FLAG_LOG
 *
 *  @sa #NL_ASSERT_PREFIX_STRING
 *  @sa #NL_ASSERT_COMPONENT_STRING
 *
 *  @sa #NL_ASSERT_FILE
 *
 */
#if !defined(NL_ASSERT_LOG)
#define NL_ASSERT_LOG(aPrefix, aName, aCondition, aLabel, aFile, aLine, aMessage)
#endif /* !defined(NL_ASSERT_LOG) */

/**
 *  @def NL_ASSERT_FILE
 *
 *  @brief
 *    This is the NULL-terminated C string literal with the fully-,
 *    partially-, or non-qualified path of the file name in which an
 *    assertion occurred (default \_\_FILE\_\_).
 *
 *  This may be used by developers to override the C preprocessor default,
 *  potentially shortening the size of string literals used for
 *  assertion file names and, consequently, decreasing the size of a
 *  particular target image.
 *
 */
#if !defined(NL_ASSERT_FILE)
#define NL_ASSERT_FILE __FILE__
#endif /* !defined(NL_ASSERT_FILE) */

/**
 *  @defgroup static-modality Static
 *  @defgroup assert-style Assert
 *  @defgroup abort-style Abort
 *  @defgroup check-style Check
 *  @defgroup verify-style Verify
 *  @defgroup precondition-style Precondition
 *  @defgroup expect-style Expect
 *  @defgroup desire-style Desire
 *  @defgroup require-style Require
 *
 */

/**
 *  @ingroup static-modality
 *
 *  @{
 *
 */

/**
 *  @def nlSTATIC_ASSERT(aCondition)
 *
 *  @brief
 *    This checks, at compile-time, for the specified condition, which
 *    is expected to commonly be true, and terminates compilation if
 *    the condition is false.
 *
 *  @note Unlike the runtime assert macros, this compile-time macro is active
 *        regardless of the state of #NL_ASSERT_PRODUCTION.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *
 *  @sa #nlSTATIC_ASSERT_PRINT
 *
 */
#define nlSTATIC_ASSERT(aCondition)                                             _nlSTATIC_ASSERT(aCondition, #aCondition)

/**
 *  @def nlSTATIC_ASSERT_PRINT(aCondition, aMessage)
 *
 *  @brief
 *    This checks, at compile-time, for the specified condition, which
 *    is expected to commonly be true, and terminates compilation if
 *    the condition is false.
 *
 *  @note Unlike the runtime assert macros, this compile-time macro is active
 *        regardless of the state of #NL_ASSERT_PRODUCTION.
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion
 *                          failure. Note, this message is not
 *                          actually emitted in any meaningful way for
 *                          non-C11 or -C++11 code. It serves to
 *                          simply comment or annotate the assertion
 *                          and to provide interface parallelism with
 *                          the run-time assertion interfaces.
 *
 *  @sa #nlSTATIC_ASSERT
 *
 */
#define nlSTATIC_ASSERT_PRINT(aCondition, aMessage)                             _nlSTATIC_ASSERT(aCondition, aMessage)

/**
 *  @}
 *
 */

/**
 *  @ingroup expect-style
 *
 *  @{
 *
 */

/**
 *  @def NL_ASSERT_EXPECT_FLAGS_DEFAULT
 *
 *  @brief
 *    This defines the default behavioral flags for expect-style
 *    exception family assertions.
 *
 *  This may be used to override #NL_ASSERT_EXPECT_FLAGS as follows:
 *
 *  @code
 *    #define NL_ASSERT_EXPECT_FLAGS NL_ASSERT_EXPECT_FLAGS_DEFAULT
 *
 *    #include <nlassert.h>
 *  @endcode
 *
 *  @sa #NL_ASSERT_EXPECT_FLAGS
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 *
 */
#define NL_ASSERT_EXPECT_FLAGS_DEFAULT                                          (NL_ASSERT_FLAG_NONE)

/**
 *  @def NL_ASSERT_EXPECT_FLAGS
 *
 *  @brief
 *    This defines the behavioral flags that govern the behavior for
 *    expect-style exception family assertions when the assertion
 *    expression evaluates to false.
 *
 *  @sa #NL_ASSERT_EXPECT_FLAGS_DEFAULT
 *
 *  @sa #NL_ASSERT_FLAG_BACKTRACE
 *  @sa #NL_ASSERT_FLAG_LOG
 *  @sa #NL_ASSERT_FLAG_TRAP
 */
#if NL_ASSERT_USE_FLAGS_DEFAULT
#define NL_ASSERT_EXPECT_FLAGS                                                  NL_ASSERT_EXPECT_FLAGS_DEFAULT
#elif !defined(NL_ASSERT_EXPECT_FLAGS)
#define NL_ASSERT_EXPECT_FLAGS                                                  (NL_ASSERT_FLAG_NONE)
#endif

/**
 *  @def nlEXPECT(aCondition, aLabel)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true, and branches to @p aLabel if the condition is
 *    false.
 *
 *  __Anticipated Assertion Firing Frequency:__ Frequent
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *
 *  @sa #NL_ASSERT_EXPECT_FLAGS
 *
 *  @sa #nlDESIRE
 *  @sa #nlREQUIRE
 *
 */
#define nlEXPECT(aCondition, aLabel)                                            __nlEXPECT(NL_ASSERT_EXPECT_FLAGS, aCondition, aLabel)

/**
 *  @def nlEXPECT_PRINT(aCondition, aLabel, aMessage)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true, and both prints @p aMessage and then branches
 *    to @p aLabel if the condition is false.
 *
 *  __Anticipated Assertion Firing Frequency:__ Frequent
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_EXPECT_FLAGS
 *
 *  @sa #nlDESIRE_PRINT
 *  @sa #nlREQUIRE_PRINT
 *
 */
#define nlEXPECT_PRINT(aCondition, aLabel, aMessage)                            __nlEXPECT_PRINT(NL_ASSERT_EXPECT_FLAGS, aCondition, aLabel, aMessage)

/**
 *  @def nlEXPECT_ACTION(aCondition, aLabel, anAction)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true, and both executes @p anAction and branches to
 *    @p aLabel if the condition is false.
 *
 *  __Anticipated Assertion Firing Frequency:__ Frequent
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_EXPECT_FLAGS
 *
 *  @sa #nlDESIRE_ACTION
 *  @sa #nlREQUIRE_ACTION
 *
 */
#define nlEXPECT_ACTION(aCondition, aLabel, anAction)                           __nlEXPECT_ACTION(NL_ASSERT_EXPECT_FLAGS, aCondition, aLabel, anAction)

/**
 *  @def nlEXPECT_ACTION_PRINT(aCondition, aLabel, anAction, aMessage)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be true, prints @p aMessage, executes @p anAction, and
 *    branches to @p aLabel if the condition is false.
 *
 *  __Anticipated Assertion Firing Frequency:__ Frequent
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_EXPECT_FLAGS
 *
 *  @sa #nlDESIRE_ACTION_PRINT
 *  @sa #nlREQUIRE_ACTION_PRINT
 *
 */
#define nlEXPECT_ACTION_PRINT(aCondition, aLabel, anAction, aMessage)           __nlEXPECT_ACTION_PRINT(NL_ASSERT_EXPECT_FLAGS, aCondition, aLabel, anAction, aMessage)

/**
 *  @def nlEXPECT_SUCCESS(aStatus, aLabel)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)), and branches to @p
 *    aLabel if the status is unsuccessful.
 *
 *  __Anticipated Assertion Firing Frequency:__ Frequent
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aStatus does not evaluate to zero
 *                          (0).
 *
 *  @sa #NL_ASSERT_EXPECT_FLAGS
 *
 *  @sa #nlDESIRE_SUCCESS
 *  @sa #nlREQUIRE_SUCCESS
 *
 */
#define nlEXPECT_SUCCESS(aStatus, aLabel)                                       __nlEXPECT_SUCCESS(NL_ASSERT_EXPECT_FLAGS, aStatus, aLabel)

/**
 *  @def nlEXPECT_SUCCESS_PRINT(aStatus, aLabel, aMessage)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)), and both prints @p
 *    aMessage and then branches to @p aLabel if the status is
 *    unsuccessful.
 *
 *  __Anticipated Assertion Firing Frequency:__ Frequent
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aStatus does not evaluate to zero
 *                          (0).
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_EXPECT_FLAGS
 *
 *  @sa #nlDESIRE_SUCCESS_PRINT
 *  @sa #nlREQUIRE_SUCCESS_PRINT
 *
 */
#define nlEXPECT_SUCCESS_PRINT(aStatus, aLabel, aMessage)                       __nlEXPECT_SUCCESS_PRINT(NL_ASSERT_EXPECT_FLAGS, aStatus, aLabel, aMessage)

/**
 *  @def nlEXPECT_SUCCESS_ACTION(aStatus, aLabel, anAction)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)), and both executes @p
 *    anAction and branches to @p aLabel if the status is unsuccessful.
 *
 *  __Anticipated Assertion Firing Frequency:__ Frequent
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aStatus does not evaluate to zero
 *                          (0).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_EXPECT_FLAGS
 *
 *  @sa #nlDESIRE_SUCCESS_ACTION
 *  @sa #nlREQUIRE_SUCCESS_ACTION
 *
 */
#define nlEXPECT_SUCCESS_ACTION(aStatus, aLabel, anAction)                      __nlEXPECT_SUCCESS_ACTION(NL_ASSERT_EXPECT_FLAGS, aStatus, aLabel, anAction)

/**
 *  @def nlEXPECT_SUCCESS_ACTION_PRINT(aStatus, aLabel, anAction, aMessage)
 *
 *  @brief
 *    This checks for the specified status, which is expected to
 *    commonly be successful (i.e. zero (0)), prints @p aMessage,
 *    executes @p anAction, and branches to @p aLabel if the status is
 *    unsuccessful.
 *
 *  __Anticipated Assertion Firing Frequency:__ Frequent
 *
 *  @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aStatus does not evaluate to zero
 *                          (0).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_EXPECT_FLAGS
 *
 *  @sa #nlDESIRE_SUCCESS_ACTION_PRINT
 *  @sa #nlREQUIRE_SUCCESS_ACTION_PRINT
 *
 */
#define nlEXPECT_SUCCESS_ACTION_PRINT(aStatus, aLabel, anAction, aMessage)      __nlEXPECT_SUCCESS_ACTION_PRINT(NL_ASSERT_EXPECT_FLAGS, aStatus, aLabel, anAction, aMessage)

/**
 *  @def nlNEXPECT(aCondition, aLabel)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false, and branches to @p aLabel if the condition is
 *    true.
 *
 *  @note This is the logical inverse of #nlEXPECT.
 *
 *  __Anticipated Assertion Firing Frequency:__ Frequent
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *
 *  @sa #NL_ASSERT_EXPECT_FLAGS
 *
 *  @sa #nlEXPECT
 *  @sa #nlNDESIRE
 *  @sa #nlNREQUIRE
 *
 */
#define nlNEXPECT(aCondition, aLabel)                                           __nlNEXPECT(NL_ASSERT_EXPECT_FLAGS, aCondition, aLabel)

/**
 *  @def nlNEXPECT_PRINT(aCondition, aLabel, aMessage)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false, and both prints @p aMessage and then branches
 *    to @p aLabel if the condition is true.
 *
 *  @note This is the logical inverse of #nlEXPECT_PRINT.
 *
 *  __Anticipated Assertion Firing Frequency:__ Frequent
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_EXPECT_FLAGS
 *
 *  @sa #nlEXPECT_PRINT
 *  @sa #nlNDESIRE_PRINT
 *  @sa #nlNREQUIRE_PRINT
 *
 */
#define nlNEXPECT_PRINT(aCondition, aLabel, aMessage)                           __nlNEXPECT_PRINT(NL_ASSERT_EXPECT_FLAGS, aCondition, aLabel, aMessage)

/**
 *  @def nlNEXPECT_ACTION(aCondition, aLabel, anAction)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false, and both executes @p anAction and branches to
 *    @p aLabel if the condition is true.
 *
 *  @note This is the logical inverse of #nlEXPECT_ACTION.
 *
 *  __Anticipated Assertion Firing Frequency:__ Frequent
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *
 *  @sa #NL_ASSERT_EXPECT_FLAGS
 *
 *  @sa #nlEXPECT_ACTION
 *  @sa #nlNDESIRE_ACTION
 *  @sa #nlNREQUIRE_ACTION
 *
 */
#define nlNEXPECT_ACTION(aCondition, aLabel, anAction)                          __nlNEXPECT_ACTION(NL_ASSERT_EXPECT_FLAGS, aCondition, aLabel, anAction)

/**
 *  @def nlNEXPECT_ACTION_PRINT(aCondition, aLabel, anAction, aMessage)
 *
 *  @brief
 *    This checks for the specified condition, which is expected to
 *    commonly be false, prints @p aMessage, executes @p anAction, and
 *    branches to @p aLabel if the condition is true.
 *
 *  @note This is the logical inverse of #nlEXPECT_ACTION_PRINT.
 *
 *  __Anticipated Assertion Firing Frequency:__ Frequent
 *
 *  @param[in]  aCondition  A Boolean expression to be evaluated.
 *  @param[in]  aLabel      The local label to which execution branches
 *                          when @p aCondition evaluates to false (i.e.
 *                          compares equal to zero).
 *  @param[in]  anAction    An expression or block to execute when the
 *                          assertion fails. This will be executed
 *                          after #NL_ASSERT_LOG() and
 *                          #NL_ASSERT_BACKTRACE() but before
 *                          #NL_ASSERT_TRAP().
 *  @param[in]  aMessage    A pointer to a NULL-terminated C string
 *                          containing a caller-specified message
 *                          further describing the assertion failure.
 *
 *  @sa #NL_ASSERT_EXPECT_FLAGS
 *
 *  @sa #nlEXPECT_ACTION_PRINT
 *  @sa #nlNDESIRE_ACTION_PRINT
 *  @sa #nlNREQUIRE_ACTION_PRINT
 *
 */
#define nlNEXPECT_ACTION_PRINT(aCondition, aLabel, anAction, aMessage)          __nlNEXPECT_ACTION_PRINT(NL_ASSERT_EXPECT_FLAGS, aCondition, aLabel, anAction, aMessage)

/**
 *  @}
 *
 */

#if !NL_ASSERT_PRODUCTION
#include "nlassert-nonproduction.h"
#else
#include "nlassert-production.h"
#endif /* !NL_ASSERT_PRODUCTION */

#endif /* NLCORE_NLASSERT_H */
