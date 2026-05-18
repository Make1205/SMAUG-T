## SPDX-License-Identifier: MIT

set(kat_srcs kat_kem.c PQCgenKAT_kem.c rng.c)

find_package(OpenSSL REQUIRED)

add_executable(${LIB_NAME1}-kat ${kat_srcs})
target_link_libraries(${LIB_NAME1}-kat PRIVATE ${LIB_NAME1} smaugt_include
                                               OpenSSL::Crypto)

add_executable(${LIB_NAME3}-kat ${kat_srcs})
target_link_libraries(${LIB_NAME3}-kat PRIVATE ${LIB_NAME3} smaugt_include
                                               OpenSSL::Crypto)

add_executable(${LIB_NAME5}-kat ${kat_srcs})
target_link_libraries(${LIB_NAME5}-kat PRIVATE ${LIB_NAME5} smaugt_include
                                               OpenSSL::Crypto)

add_executable(${LIB_NAMET}-kat ${kat_srcs})
target_link_libraries(${LIB_NAMET}-kat PRIVATE ${LIB_NAMET} smaugt_include
                                               OpenSSL::Crypto)
