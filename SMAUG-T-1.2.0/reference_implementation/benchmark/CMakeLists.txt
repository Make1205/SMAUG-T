## SPDX-License-Identifier: MIT

set(BENCH_NAME1 ${PROJECT_NAME}-mode1)
add_executable(
  ${BENCH_NAME1}-speed
  ${CMAKE_CURRENT_LIST_DIR}/speed.c ${CMAKE_CURRENT_LIST_DIR}/cpucycles.c
  ${CMAKE_CURRENT_LIST_DIR}/speed_print.c)
add_executable(${BENCH_NAME1}-benchmark ${CMAKE_CURRENT_LIST_DIR}/PQC_bench.c
                                        ${PROJECT_SOURCE_DIR}/src/randombytes.c)
target_link_libraries(${BENCH_NAME1}-speed ${LIB_NAME1} smaugt_include)
target_link_libraries(${BENCH_NAME1}-benchmark ${LIB_NAME1} smaugt_include)

set(BENCH_NAME3 ${PROJECT_NAME}-mode3)
add_executable(
  ${BENCH_NAME3}-speed
  ${CMAKE_CURRENT_LIST_DIR}/speed.c ${CMAKE_CURRENT_LIST_DIR}/cpucycles.c
  ${CMAKE_CURRENT_LIST_DIR}/speed_print.c)
add_executable(${BENCH_NAME3}-benchmark ${CMAKE_CURRENT_LIST_DIR}/PQC_bench.c
                                        ${PROJECT_SOURCE_DIR}/src/randombytes.c)
target_link_libraries(${BENCH_NAME3}-speed ${LIB_NAME3} smaugt_include)
target_link_libraries(${BENCH_NAME3}-benchmark ${LIB_NAME3} smaugt_include)

set(BENCH_NAME5 ${PROJECT_NAME}-mode5)
add_executable(
  ${BENCH_NAME5}-speed
  ${CMAKE_CURRENT_LIST_DIR}/speed.c ${CMAKE_CURRENT_LIST_DIR}/cpucycles.c
  ${CMAKE_CURRENT_LIST_DIR}/speed_print.c)
add_executable(${BENCH_NAME5}-benchmark ${CMAKE_CURRENT_LIST_DIR}/PQC_bench.c
                                        ${PROJECT_SOURCE_DIR}/src/randombytes.c)
target_link_libraries(${BENCH_NAME5}-speed ${LIB_NAME5} smaugt_include)
target_link_libraries(${BENCH_NAME5}-benchmark ${LIB_NAME5} smaugt_include)

set(BENCH_NAMET ${PROJECT_NAME}-modet)
add_executable(
  ${BENCH_NAMET}-speed
  ${CMAKE_CURRENT_LIST_DIR}/speed.c ${CMAKE_CURRENT_LIST_DIR}/cpucycles.c
  ${CMAKE_CURRENT_LIST_DIR}/speed_print.c)
add_executable(${BENCH_NAMET}-benchmark ${CMAKE_CURRENT_LIST_DIR}/PQC_bench.c
                                        ${PROJECT_SOURCE_DIR}/src/randombytes.c)
target_link_libraries(${BENCH_NAMET}-speed ${LIB_NAMET} smaugt_include)
target_link_libraries(${BENCH_NAMET}-benchmark ${LIB_NAMET} smaugt_include)
