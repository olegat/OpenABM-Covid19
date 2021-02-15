# Description:
#
#   The primary intend of this file to facilitate debugging on Windows. This
#   file will generate build files to compile covid19ibm.exe with debug symbols.
#
# GSL:
#
#   To compile with this file, you'll need a native installation of GSL.
#
#   First, set the GSL_ROOT_DIR environment variable to where to want to install
#   GSL. CMake will use this environment variable to locate GSL. Example:
#
#   > set GSL_ROOT_DIR=D:\gsl\2.5.0\Debug32
#
#   Second, you'll need to build & install GSL with CMake. Here's an example of
#   how to install GSL into %GSL_ROOT_DIR% using Visual Studio 2017:
#
#   > git clone https://github.com/ampl/gsl/
#   > cd gsl
#   > git checkout v2.5.0
#   > cmake -G "Visual Studio 15 2017" -AWin32 -S. -Bbuild -DGSL_INSTALL_MULTI_CONFIG=OFF -DCMAKE_INSTALL_PREFIX=%GSL_ROOT_DIR% -DBUILD_SHARED_LIBS=OFF
#   > cmake --build build --config Debug --target gsl
#   > cmake --build build --config Debug --target install
#
# LLVM/Ninja:
#
#   If you want to build with LLVM & Ninja, make sure that clang.exe and
#   ninja.exe included in your %PATH% environment variable. If you're testing
#   Windows 32-bit, make sure to install the 32-bit compiler of Clang.
#
#   Downloads:
#
#     LLVM 11.0.1 Windows (32-bit):
#       https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.1/LLVM-11.0.1-win32.exe
#
#     LLVM 11.0.1 Windows (64-bit):
#       https://github.com/llvm/llvm-project/releases/download/llvmorg-11.0.1/LLVM-11.0.1-win64.exe
#
# Building:
#
#   Example with LLVM/Ninja (32-bit):
#
#   > set PATH=C:\Program Files (x86)\LLVM\bin;C:\Program Files\Ninja\;%PATH%
#   > set GSL_ROOT_DIR=D:\gsl\2.5.0\Debug32
#   > cmake -GNinja -S. -Bbuild
#   > cmake --build build
#
#   Example with Visual Studio (32-bit):
#
#   > set GSL_ROOT_DIR=D:\gsl\2.5.0\Debug32
#   > cmake -G "Visual Studio 15 2017" -A Win32 -S. -Bbuild
#   > cmake --build build --config Debug
#
cmake_minimum_required(VERSION 3.16)

project(OpenABMCovid19)

find_package(GSL REQUIRED)

option(USE_ASAN "Use LLVM Address Sanitizer (-fsanitize=address)" OFF)

add_executable(covid19ibm
  src/constant.h
  src/constant.c
  src/demographics.c
  src/demographics.h
  src/disease.c
  src/disease.h
  src/doctor.c
  src/doctor.h
  src/hospital.c
  src/hospital.h
  src/individual.c
  src/individual.h
  src/input.c
  src/input.h
  src/interventions.c
  src/interventions.h
  src/list.c
  src/list.h
  src/main.c
  src/model.c
  src/model.h
  src/network.c
  src/network.h
  src/nurse.c
  src/nurse.h
  src/params.c
  src/params.h
  src/structure.h
  src/utilities.c
  src/utilities.h
  src/ward.c
  src/ward.h
)

target_link_libraries(covid19ibm
  PUBLIC
    GSL::gsl
    GSL::gslcblas
)

if (USE_ASAN)
  target_link_options(   covid19ibm PRIVATE -fsanitize=address)
  target_compile_options(covid19ibm PRIVATE -fsanitize=address)
endif()

if (WIN32)
  # port `clock_gettime` on Windows MSVC
  target_compile_definitions(covid19ibm PRIVATE -DINCLUDE_CLOCK_WIN32=1)
  target_sources(covid19ibm
    PRIVATE
      src/clock_win32.h
      src/clock_win32.c
  )

  if( USE_ASAN AND "${CMAKE_C_COMPILER_ID}" MATCHES Clang)
    # Possible CMake bug:
    #
    # Option `--dependent-lib=msvcrtd` (from Modules/Platform/Windows-Clang.cmake)
    # causes linker errors when used with -fsanitize=address so remove this flag
    message("-- Hack: removing link option --dependent-lib=msvcrtd (incompatible with -fsanitize=address)")
    string(REPLACE "--dependent-lib=msvcrtd" ""
      CMAKE_C_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreadedDebugDLL
      "${CMAKE_C_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreadedDebugDLL}")

    # Possibly another CMake bug:
    #
    # Linking to these default libs with `-fsanitize=address` causes linker errors
    # because of duplicates like _calloc and _free.
    target_link_options(covid19ibm PRIVATE
      -lclang_rt.asan-i386.lib
      -Wl,/nodefaultlib:libucrt.lib
      -Wl,/nodefaultlib:libucrtd.lib
    )

    # Looks like an LLVM bug:
    #
    # Some C Runtime functions like _fopen, _strcat, _ceil, ... are missing in
    # libasan on Windows.
    target_sources(covid19ibm
      PRIVATE
        src/asan_crt_win32.c
    )
  endif()
endif()
