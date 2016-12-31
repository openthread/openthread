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

IF NOT "%Platform%"=="x64" GOTO :EOF

pushd %APPVEYOR_BUILD_FOLDER%\build\bin\%Platform2%\%Configuration%\sys

REM Install the certifications to the cert stores
certutil -addstore root otLwf.cer
certutil -addstore TrustedPublisher otLwf.cer

cd otLwf

REM Install the NDIS LWF driver, otLwf.sys

netcfg.exe -v -l otlwf.inf -c s -i otLwf

popd