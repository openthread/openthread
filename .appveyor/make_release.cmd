REM
REM  Copyright (c) 2016, The OpenThread Authors.
REM  All rights reserved.
REM
REM  Redistribution and use in source and binary forms, with or without
REM  modification, are permitted provided that the following conditions are met:
REM  1. Redistributions of source code must retain the above copyright
REM     notice, this list of conditions and the following disclaimer.
REM  2. Redistributions in binary form must reproduce the above copyright
REM     notice, this list of conditions and the following disclaimer in the
REM     documentation and/or other materials provided with the distribution.
REM  3. Neither the name of the copyright holder nor the
REM     names of its contributors may be used to endorse or promote products
REM     derived from this software without specific prior written permission.
REM
REM  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
REM  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
REM  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
REM  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
REM  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
REM  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
REM  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
REM  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
REM  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
REM  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
REM  POSSIBILITY OF SUCH DAMAGE.
REM

pushd %APPVEYOR_BUILD_FOLDER%

REM Make the release directories
mkdir release
mkdir release\include
mkdir release\include\openthread\
mkdir release\include\openthread\missing
mkdir release\include\openthread\platform
mkdir release\libs
mkdir release\symbols
mkdir release\symbols\TraceFormat

REM Copy the relavant include headers

copy include\openthread\commissioner.h release\include\openthread
copy include\openthread\dataset.h release\include\openthread
copy include\openthread\error.h release\include\openthread
copy include\openthread\instance.h release\include\openthread
copy include\openthread\ip6.h release\include\openthread
copy include\openthread\joiner.h release\include\openthread
copy include\openthread\link.h release\include\openthread
copy include\openthread\message.h release\include\openthread
copy include\openthread\netdata.h release\include\openthread
copy include\openthread\thread.h release\include\openthread
copy include\openthread\types.h release\include\openthread
copy include\openthread\platform\toolchain.h release\include\openthread\platform
copy src\missing\stdbool\stdbool.h release\include\openthread\missing
copy src\missing\stdint\stdint.h release\include\openthread\missing

REM Copy the relavant binaries

copy build\bin\%Platform2%\%Configuration%\sys\otlwf\* release
copy build\bin\%Platform2%\%Configuration%\sys\otlwf.cer release
copy build\bin\%Platform2%\%Configuration%\sys\otlwf.pdb release\symbols
copy build\bin\%Platform2%\%Configuration%\sys\ottmp\* release
copy build\bin\%Platform2%\%Configuration%\sys\ottmp.cer release
copy build\bin\%Platform2%\%Configuration%\sys\ottmp.pdb release\symbols
copy build\bin\%Platform2%\%Configuration%\dll\otApi.dll release
copy build\bin\%Platform2%\%Configuration%\dll\otApi.lib release\libs
copy build\bin\%Platform2%\%Configuration%\dll\otApi.pdb release\symbols
copy build\bin\%Platform2%\%Configuration%\dll\otNodeApi.dll release
copy build\bin\%Platform2%\%Configuration%\dll\otNodeApi.lib release\libs
copy build\bin\%Platform2%\%Configuration%\dll\otNodeApi.pdb release\symbols
copy build\bin\%Platform2%\%Configuration%\exe\otCli.exe release
copy build\bin\%Platform2%\%Configuration%\exe\otCli.pdb release\symbols
copy build\bin\%Platform2%\%Configuration%\exe\otTestRunner.exe release
copy build\bin\%Platform2%\%Configuration%\exe\otTestRunner.pdb release\symbols

REM Copy the tools

copy tools\windows\* release
copy tools\windows\%Platform%\otInstall.exe release
copy "C:\Program Files (x86)\Windows Kits\10\Tools\%Platform%\devcon.exe" release

REM Generate the trace format files to decode the WPP logs

"C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Bin\x64\TracePdb.exe" -f release\symbols\*.pdb -p release\symbols\TraceFormat

popd