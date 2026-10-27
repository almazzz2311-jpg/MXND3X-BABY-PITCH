@echo off
setlocal
cd /d "%~dp0"
where cmake >nul 2>&1
if errorlevel 1 (
  echo ERROR: Install CMake 3.24 or newer and add it to PATH.
  exit /b 1
)
where git >nul 2>&1
if errorlevel 1 (
  echo ERROR: Install Git for Windows and add it to PATH.
  exit /b 1
)
cmake -S . -B build-windows -G "Visual Studio 17 2022" -A x64 -DCMAKE_CONFIGURATION_TYPES=Release -DBUILD_TESTING=ON
if errorlevel 1 (
  echo ERROR: Configure failed. Install Visual Studio 2022 Desktop development with C++ and Windows SDK. Check Internet access to GitHub.
  exit /b 1
)
cmake --build build-windows --config Release --parallel 2
if errorlevel 1 (
  echo ERROR: Compilation failed. See the compiler output above.
  exit /b 1
)
ctest --test-dir build-windows -C Release --output-on-failure
if errorlevel 1 (
  echo ERROR: Validation failed. Distribution has not been updated.
  exit /b 1
)
if not exist "build-windows\MXND3XBabyVoice_artefacts\Release\VST3\MXND3X BABY VOICE.vst3\Contents\x86_64-win\MXND3X BABY VOICE.vst3" (
  echo ERROR: Expected Windows x64 VST3 binary is missing.
  exit /b 1
)
if not exist dist mkdir dist
robocopy "build-windows\MXND3XBabyVoice_artefacts\Release\VST3\MXND3X BABY VOICE.vst3" "dist\MXND3X BABY VOICE.vst3" /E /PURGE /NFL /NDL /NJH /NJS
if errorlevel 8 (
  echo ERROR: Could not copy VST3 bundle into dist.
  exit /b 1
)
echo SUCCESS: dist\MXND3X BABY VOICE.vst3
 echo Copy this complete folder to C:\Program Files\Common Files\VST3\ then rescan FL Studio Plugin Manager.
exit /b 0
