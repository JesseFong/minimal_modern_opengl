@echo off

set CustomRoot=%cd%
set BuildTarget=%CustomRoot%\minimal_bindless_gl.cpp

set CompilerFlags=-Od -MTd -nologo -fp:fast -fp:except- -GR- -Zo -Oi -WX -W4 -FC -Zi -EHa 
set WarningFlags=-wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -wd4457 -wd4456 -wd4312 -wd4477 -wd4838 -wd4244 -wd4091
set CommonCompilerFlags= %CompilerFlags% %WarningFlags%
set CommonLinkerFlags= -incremental:no -opt:ref

IF NOT EXIST %CustomRoot%\build mkdir %CustomRoot%\build
pushd %CustomRoot%\build

cl %CommonCompilerFlags% /I%Includes%  -D_CRT_SECURE_NO_WARNINGS %BuildTarget%  /link  %CommonLinkerFlags%

popd