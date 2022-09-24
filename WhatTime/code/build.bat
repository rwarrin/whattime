@echo off

SET CompilerFlags=/nologo /Z7 /Od /Oi /fp:fast /GS- /Ob0 /FAc
SET LinkerFlags=

IF NOT EXIST build mkdir build

pushd build

cl %CompilerFlags% ../WhatTime/code/whattime.cpp /link %LinkerFlags% advapi32.lib

popd
