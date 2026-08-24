@echo off

mkdir build
pushd build
:: -Zi genera debug info
:: -FC per avere full pathnames nei diagnostics
cl -FC -Zi ..\code\win32_handmade.cpp user32.lib Gdi32.lib
popd