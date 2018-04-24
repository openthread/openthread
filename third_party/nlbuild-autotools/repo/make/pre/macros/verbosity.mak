#
#    Copyright 2017-2018 Nest Labs Inc. All Rights Reserved.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
#

#
#    Description:
#      This file is a make "header" or pre make header that defines make macros
#      for controlling build verbosity.
#

#
# Verbosity
#

NL_DEFAULT_VERBOSITY             ?= 0

NL_V_AT                           = $(NL_V_AT_$(V))
NL_V_AT_                          = $(NL_V_AT_$(NL_DEFAULT_VERBOSITY))
NL_V_AT_0                         = @
NL_V_AT_1                         = 

NL_V_BOOTSTRAP_ALL                = $(NL_V_BOOTSTRAP_ALL_$(V))
NL_V_BOOTSTRAP_ALL_               = $(NL_V_BOOTSTRAP_ALL_$(NL_DEFAULT_VERBOSITY))
NL_V_BOOTSTRAP_ALL_0              = @echo "  BOOTSTRAP    all";
NL_V_BOOTSTRAP_ALL_1              = 

NL_V_BOOTSTRAP_CONFIG             = $(NL_V_BOOTSTRAP_CONFIG_$(V))
NL_V_BOOTSTRAP_CONFIG_            = $(NL_V_BOOTSTRAP_CONFIG_$(NL_DEFAULT_VERBOSITY))
NL_V_BOOTSTRAP_CONFIG_0           = @echo "  BOOTSTRAP    config";
NL_V_BOOTSTRAP_CONFIG_1           = 

NL_V_BOOTSTRAP_MAKE               = $(NL_V_BOOTSTRAP_MAKE_$(V))
NL_V_BOOTSTRAP_MAKE_              = $(NL_V_BOOTSTRAP_MAKE_$(NL_DEFAULT_VERBOSITY))
NL_V_BOOTSTRAP_MAKE_0             = @echo "  BOOTSTRAP    make";
NL_V_BOOTSTRAP_MAKE_1             = 

NL_V_CONFIGURE                    = $(NL_V_CONFIGURE_$(V))
NL_V_CONFIGURE_                   = $(NL_V_CONFIGURE_$(NL_DEFAULT_VERBOSITY))
NL_V_CONFIGURE_0                  = @echo "  CONFIGURE";
NL_V_CONFIGURE_1                  = 

NL_V_GIT_INIT                     = $(NL_V_GIT_INIT_$(V))
NL_V_GIT_INIT_                    = $(NL_V_GIT_INIT_$(NL_DEFAULT_VERBOSITY))
NL_V_GIT_INIT_0                   = @echo "  GIT INIT     $(@)";
NL_V_GIT_INIT_1                   = 

NL_V_MAKE                         = $(NL_V_MAKE_$(V))
NL_V_MAKE_                        = $(NL_V_MAKE_$(NL_DEFAULT_VERBOSITY))
NL_V_MAKE_0                       = @echo "  MAKE         $(@)";
NL_V_MAKE_1                       = 

NL_V_MKDIR_P                      = $(NL_V_MKDIR_P_$(V))
NL_V_MKDIR_P_                     = $(NL_V_MKDIR_P_$(NL_DEFAULT_VERBOSITY))
NL_V_MKDIR_P_0                    = @echo "  MKDIR        $(1)";
NL_V_MKDIR_P_1                    = 

NL_V_RMDIR                        = $(NL_V_RMDIR_$(V))
NL_V_RMDIR_                       = $(NL_V_RMDIR_$(NL_DEFAULT_VERBOSITY))
NL_V_RMDIR_0                      = @echo "  RMDIR        $(1)";
NL_V_RMDIR_1                      = 
