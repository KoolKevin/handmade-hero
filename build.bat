@echo off

mkdir build
pushd build
:: -Zi genera debug info
cl -Zi ..\code\win32_handmade.cpp user32.lib Gdi32.lib
popd