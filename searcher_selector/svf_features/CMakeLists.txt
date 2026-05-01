cmake_minimum_required(VERSION 3.13.4)
project(svf_extract_features C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

# SVF was built with -fno-rtti / -fno-exceptions, so we must match.
add_compile_options(-fno-rtti -fno-exceptions -Wall -Wno-deprecated-declarations)

# --- LLVM ----------------------------------------------------------------
find_package(LLVM 14 REQUIRED CONFIG HINTS ${LLVM_DIR} $ENV{LLVM_DIR})
message(STATUS "LLVM ${LLVM_PACKAGE_VERSION} from ${LLVM_INCLUDE_DIRS}")
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# --- SVF -----------------------------------------------------------------
# Expect SVF_DIR to point at the SVF source tree (its build dir contains the
# static libs).  We default to ~/src/SVF.
if(NOT DEFINED SVF_DIR)
  set(SVF_DIR "$ENV{HOME}/src/SVF" CACHE PATH "Path to SVF source tree")
endif()
set(SVF_BUILD_DIR "${SVF_DIR}/Release-build")

include_directories(SYSTEM
    ${SVF_DIR}/svf/include
    ${SVF_DIR}/svf-llvm/include
    ${SVF_BUILD_DIR}/include)

# --- Z3 ------------------------------------------------------------------
find_library(Z3_LIB NAMES z3 HINTS /usr/lib/x86_64-linux-gnu /usr/lib REQUIRED)

# --- LLVM libraries ------------------------------------------------------
llvm_map_components_to_libnames(LLVM_LIBS
    Core Support Analysis IRReader BitReader BitWriter TransformUtils Passes)

# --- Target --------------------------------------------------------------
add_executable(extract_features extract_features.cpp)
target_link_libraries(extract_features PRIVATE
    ${SVF_BUILD_DIR}/svf-llvm/libSvfLLVM.a
    ${SVF_BUILD_DIR}/svf/libSvfCore.a
    ${LLVM_LIBS}
    ${Z3_LIB}
    pthread dl)
