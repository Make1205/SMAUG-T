## SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.18)
project(
  smaugt
  LANGUAGES C
  VERSION 1.0.0
  DESCRIPTION "SMAUG-T Reference Implementation")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin/)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(srcs
    ${PROJECT_SOURCE_DIR}/src/cbd.c
    ${PROJECT_SOURCE_DIR}/src/ciphertext.c
    ${PROJECT_SOURCE_DIR}/src/dg.c
    ${PROJECT_SOURCE_DIR}/src/hash.c
    ${PROJECT_SOURCE_DIR}/src/hwt.c
    ${PROJECT_SOURCE_DIR}/src/indcpa.c
    ${PROJECT_SOURCE_DIR}/src/kem.c
    ${PROJECT_SOURCE_DIR}/src/key.c
    ${PROJECT_SOURCE_DIR}/src/pack.c
    ${PROJECT_SOURCE_DIR}/src/packring.c
    ${PROJECT_SOURCE_DIR}/src/poly.c
    ${PROJECT_SOURCE_DIR}/src/toomcook.c
    ${PROJECT_SOURCE_DIR}/src/verify.c)

add_library(smaugt_include INTERFACE)
target_include_directories(smaugt_include
                           INTERFACE ${PROJECT_SOURCE_DIR}/include)

add_library(smaugt_rng OBJECT ${PROJECT_SOURCE_DIR}/src/randombytes.c)
target_link_libraries(smaugt_rng PRIVATE smaugt_include)

add_library(smaugt_fips202 OBJECT ${PROJECT_SOURCE_DIR}/src/fips202.c)
target_link_libraries(smaugt_fips202 PRIVATE smaugt_include)
set_target_properties(smaugt_fips202 PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(LIB_NAME1 ${PROJECT_NAME}-mode1)
add_library(${LIB_NAME1} SHARED ${srcs})
target_compile_definitions(${LIB_NAME1} PUBLIC SMAUGT_CONFIG_MODE=SMAUGT_MODE1)
target_link_libraries(${LIB_NAME1} PRIVATE smaugt_rng smaugt_fips202
                                           smaugt_include)

set(LIB_NAME3 ${PROJECT_NAME}-mode3)
add_library(${LIB_NAME3} SHARED ${srcs})
target_compile_definitions(${LIB_NAME3} PUBLIC SMAUGT_CONFIG_MODE=SMAUGT_MODE3)
target_link_libraries(${LIB_NAME3} PRIVATE smaugt_rng smaugt_fips202
                                           smaugt_include)

set(LIB_NAME5 ${PROJECT_NAME}-mode5)
add_library(${LIB_NAME5} SHARED ${srcs})
target_compile_definitions(${LIB_NAME5} PUBLIC SMAUGT_CONFIG_MODE=SMAUGT_MODE5)
target_link_libraries(${LIB_NAME5} PRIVATE smaugt_rng smaugt_fips202
                                           smaugt_include)

set(LIB_NAMET ${PROJECT_NAME}-modet)
add_library(${LIB_NAMET} SHARED ${srcs})
target_compile_definitions(${LIB_NAMET} PUBLIC SMAUGT_CONFIG_MODE=SMAUGT_MODET)
target_link_libraries(${LIB_NAMET} PRIVATE smaugt_rng smaugt_fips202
                                           smaugt_include)

option(SMAUGT_BUILD_TEST "Build the test suite" ON)

if(${SMAUGT_BUILD_TEST})
  include(CTest)
  add_subdirectory(test)
endif()

add_subdirectory(kat)
add_subdirectory(benchmark)
