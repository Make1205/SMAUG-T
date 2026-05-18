## SPDX-License-Identifier: MIT

add_executable(${LIB_NAME1}-main main.c)
target_link_libraries(${LIB_NAME1}-main PRIVATE ${LIB_NAME1} smaugt_include
                                                smaugt_rng)

add_executable(${LIB_NAME3}-main main.c)
target_link_libraries(${LIB_NAME3}-main PRIVATE ${LIB_NAME3} smaugt_include
                                                smaugt_rng)
add_executable(${LIB_NAME5}-main main.c)
target_link_libraries(${LIB_NAME5}-main PRIVATE ${LIB_NAME5} smaugt_include
                                                smaugt_rng)

add_executable(${LIB_NAMET}-main main.c)
target_link_libraries(${LIB_NAMET}-main PRIVATE ${LIB_NAMET} smaugt_include
                                                smaugt_rng)
enable_language(CXX)

include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)
cpmaddpackage(
  NAME
  googletest
  GITHUB_REPOSITORY
  google/googletest
  VERSION
  1.17.0
  OPTIONS
  "INSTALL_GTEST OFF")
# For Windows: prevents GTest from overriding your compiler's runtime library
# settings
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)

include(GoogleTest)

add_executable(${LIB_NAME1}-googletest UnitTest.cpp)
target_link_libraries(${LIB_NAME1}-googletest
                      PRIVATE GTest::gtest_main ${LIB_NAME1} smaugt_include)
gtest_discover_tests(${LIB_NAME1}-googletest XML_OUTPUT_DIR ctest_res)

add_executable(${LIB_NAME3}-googletest UnitTest.cpp)
target_link_libraries(${LIB_NAME3}-googletest
                      PRIVATE GTest::gtest_main ${LIB_NAME3} smaugt_include)
gtest_discover_tests(${LIB_NAME3}-googletest XML_OUTPUT_DIR ctest_res)

add_executable(${LIB_NAME5}-googletest UnitTest.cpp)
target_link_libraries(${LIB_NAME5}-googletest
                      PRIVATE GTest::gtest_main ${LIB_NAME5} smaugt_include)
gtest_discover_tests(${LIB_NAME5}-googletest XML_OUTPUT_DIR ctest_res)

add_executable(${LIB_NAMET}-googletest UnitTest.cpp)
target_link_libraries(${LIB_NAMET}-googletest
                      PRIVATE GTest::gtest_main ${LIB_NAMET} smaugt_include)
gtest_discover_tests(${LIB_NAMET}-googletest XML_OUTPUT_DIR ctest_res)
