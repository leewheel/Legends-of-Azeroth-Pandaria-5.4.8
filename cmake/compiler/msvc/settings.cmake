# Copyright (C) 2011-2016 Project SkyFire <http://www.projectskyfire.org/
# Copyright (C) 2008-2016 TrinityCore <http://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# set up output paths for executable binaries (.exe-files, and .dll-files on DLL-capable platforms)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(MSVC_EXPECTED_VERSION 19.37.32826.1)

if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS MSVC_EXPECTED_VERSION)
  message(FATAL_ERROR "MSVC: SkyFire requires version ${MSVC_EXPECTED_VERSION} (MSVC 2022) to build but found ${CMAKE_CXX_COMPILER_VERSION}")
endif()

# set up output paths ofr static libraries etc (commented out - shown here as an example only)
#set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
#set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if(PLATFORM EQUAL 64)
  # This definition is necessary to work around a bug with Intellisense described
  # here: http://tinyurl.com/2cb428. Syntax highlighting is important for proper
  # debugger functionality.
  add_definitions("-D_WIN64")
  message(STATUS "MSVC: 64-bit platform, enforced -D_WIN64 parameter")

  #Enable extended object support for debug compiles on X64 (not required on X86)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj")
  message(STATUS "MSVC: Enabled extended object-support for debug-compiles")
else()
  # mark 32 bit executables large address aware so they can use > 2GB address space
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
  message(STATUS "MSVC: Enabled large address awareness")

  add_definitions(/arch:SSE2)
  message(STATUS "MSVC: Enabled SSE2 support")
endif()

# Set build-directive (used in core to tell which buildtype we used)
if("${CMAKE_MAKE_PROGRAM}" MATCHES "MSBuild")
  add_compile_options(-D_BUILD_DIRECTIVE="$(ConfigurationName)")
else()
  add_compile_options(-D_BUILD_DIRECTIVE="$<CONFIG>")
endif()

# multithreaded compiling on VS
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")

# Define _CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES - eliminates the warning by changing the strcpy call to strcpy_s, which prevents buffer overruns
add_definitions(-D_CRT_SECURE_CPP_OVERLOAD_STANDARD_NAMES)
message(STATUS "MSVC: Overloaded standard names")

# Ignore warnings about older, less secure functions
add_definitions(-D_CRT_SECURE_NO_WARNINGS)
message(STATUS "MSVC: Disabled NON-SECURE warnings")

#Ignore warnings about POSIX deprecation
add_definitions(-D_CRT_NONSTDC_NO_WARNINGS)
message(STATUS "MSVC: Disabled POSIX warnings")

if(NOT WITH_WARNINGS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4996 /wd4355 /wd4244 /wd4985 /wd4267 /wd4619 /wd4512")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4996 /wd4355 /wd4244 /wd4985 /wd4267 /wd4619 /wd4512")
  message(STATUS "MSVC: Disabled generic compiletime warnings")
endif()

# Specify the maximum PreCompiled Header memory allocation limit
# Fixes a compiler-problem when using PCH - the /Ym flag is adjusted by the compiler in MSVC2012, hence we need to set an upper limit with /Zm to avoid discrepancies)
# (And yes, this is a verified , unresolved bug with MSVC... *sigh*)
string(REGEX REPLACE "/Zm[0-9]+ *" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zm500")

# Enable and treat as errors the following warnings to easily detect virtual function signature failures:
# 'function' : member function does not override any base class virtual member function
# 'virtual_function' : no override available for virtual member function from base 'class'; function is hidden
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /we4263 /we4264")

macro(ApplyFlags variable)
  string(REGEX REPLACE "/INCREMENTAL *" "/INCREMENTAL:NO" ${variable} "${${variable}}")
endmacro()

if (BUILD_DEV)
  ApplyFlags(CMAKE_EXE_LINKER_FLAGS_DEBUG)
  ApplyFlags(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO)
  ApplyFlags(CMAKE_SHARED_LINKER_FLAGS_DEBUG)
  ApplyFlags(CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zc:inline")
endif()
