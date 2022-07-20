:: Copyright (c) 2022, The OpenThread Authors.
:: All rights reserved.
::
:: Redistribution and use in source and binary forms, with or without
:: modification, are permitted provided that the following conditions are met:
:: 1. Redistributions of source code must retain the above copyright
::    notice, this list of conditions and the following disclaimer.
:: 2. Redistributions in binary form must reproduce the above copyright
::    notice, this list of conditions and the following disclaimer in the
::    documentation and/or other materials provided with the distribution.
:: 3. Neither the name of the copyright holder nor the
::    names of its contributors may be used to endorse or promote products
::    derived from this software without specific prior written permission.
::
:: THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
:: AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
:: IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
:: ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
:: LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
:: CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
:: SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
:: INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
:: CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
:: ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
:: POSSIBILITY OF SUCH DAMAGE.
::

xcopy /E /Y Thread_Harness %systemdrive%\GRL\Thread1.2\Thread_Harness
copy /Y ..\..\harness-thci\OpenThread.py %systemdrive%\GRL\Thread1.2\Thread_Harness\THCI
xcopy /E /Y ..\posix\sniffer_sim\proto %systemdrive%\GRL\Thread1.2\Thread_Harness\simulation\Sniffer\proto

%systemdrive%\GRL\Thread1.2\Python27\python.exe -m pip install --upgrade pip
%systemdrive%\GRL\Thread1.2\Python27\python.exe -m pip install -r requirements.txt

set BASEDIR=%systemdrive%\GRL\Thread1.2\Thread_Harness

%systemdrive%\GRL\Thread1.2\Python27\python.exe -m grpc_tools.protoc -I%BASEDIR% --python_out=%BASEDIR% --grpc_python_out=%BASEDIR% simulation/Sniffer/proto/sniffer.proto

pause
