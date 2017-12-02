cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(clt13 LANGUAGES C)

include(GNUInstallDirs)

set(clt13_SOURCES
  src/clt13.c
  src/crt_tree.c
  src/estimates.c
  )
set(clt13_HEADERS
  src/clt13.h
  src/crt_tree.h
  src/estimates.h
  )

add_library(clt13 SHARED ${clt13_SOURCES})
target_include_directories(clt13 PRIVATE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
target_link_libraries(clt13 PUBLIC gmp aesrand)

include(FindOpenMP)
target_compile_options(clt13 PRIVATE
  -Wall -Wextra -Wpedantic -Werror -std=gnu11 -march=native ${OpenMP_C_FLAGS})
target_link_libraries(clt13 INTERFACE "${OpenMP_C_FLAGS}")

install(TARGETS clt13 LIBRARY DESTINATION lib)
install(FILES src/clt13.h DESTINATION include)

# Test files

add_executable(test_clt test/test_clt.c)
add_executable(test_clt_state_new test/test_clt_state_new.c)
target_include_directories(test_clt PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_clt PRIVATE clt13 aesrand)
target_include_directories(test_clt_state_new PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(test_clt_state_new PRIVATE clt13 aesrand)

enable_testing()
add_test(NAME test_clt COMMAND test_clt)
add_test(NAME test_clt_state_new COMMAND test_clt_state_new)