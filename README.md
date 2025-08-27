

# Build
In PowerShell:
```
$env:Path = "E:\Qt\Tools\mingw1310_64\bin;E:\Qt\6.9.1\mingw_64\bin;$env:Path"

cmake -S . -B build -G "MinGW Makefiles" `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_PREFIX_PATH="E:/Qt/6.9.1/mingw_64" `
  -DCMAKE_C_COMPILER="E:/Qt/Tools/mingw1310_64/bin/gcc.exe" `
  -DCMAKE_CXX_COMPILER="E:/Qt/Tools/mingw1310_64/bin/g++.exe"

cmake --build build --config Release -j
```