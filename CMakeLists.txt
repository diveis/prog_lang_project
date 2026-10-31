cmake_minimum_required(VERSION 3.10)
project(hashwatch VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# Библиотека со всей логикой (без main.c)
file(GLOB HW_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/src/*.c)
list(REMOVE_ITEM HW_SOURCES ${CMAKE_SOURCE_DIR}/src/main.c)

add_library(hashwatch_core STATIC ${HW_SOURCES})
target_include_directories(hashwatch_core PUBLIC src)
target_compile_definitions(hashwatch_core PUBLIC _POSIX_C_SOURCE=200809L)
if(NOT MSVC)
  target_compile_options(hashwatch_core PUBLIC -Wall -Wextra -Wpedantic)
endif()

if(EXISTS ${CMAKE_SOURCE_DIR}/src/main.c)
  add_executable(hashwatch src/main.c)
  target_link_libraries(hashwatch PRIVATE hashwatch_core)
  install(TARGETS hashwatch RUNTIME DESTINATION bin)
endif()

# Тесты
enable_testing()
file(GLOB HW_TESTS CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/tests/test_*.c)
foreach(test_src ${HW_TESTS})
  get_filename_component(test_name ${test_src} NAME_WE)
  add_executable(${test_name} ${test_src})
  target_link_libraries(${test_name} PRIVATE hashwatch_core)
  add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

if(TARGET hashwatch AND EXISTS ${CMAKE_SOURCE_DIR}/tests/integration.sh)
  add_test(NAME integration
           COMMAND sh ${CMAKE_SOURCE_DIR}/tests/integration.sh $<TARGET_FILE:hashwatch>)
endif()
