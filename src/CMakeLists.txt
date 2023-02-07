# CMake 最低版本号要求
cmake_minimum_required (VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)
aux_source_directory(. DIR_SRCS)

#

if(DEFINED VCPKG_PATH)
  message("enable ssl")
  set(CMAKE_TOOLCHAIN_FILE ${VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake)
  set(VCPKG_TARGET_TRIPLET ${VCPKG_TARGET_TRIPLET})
  set(VCPKG_MANIFEST_MODE OFF)
  message(CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE})
  project (sion)
else()
  message("disable ssl")
  project (sion)
endif()

add_executable(sion ${DIR_SRCS})
if(NOT WIN32)
  target_link_libraries(sion PRIVATE pthread)
endif()


if(DEFINED VCPKG_PATH)
  target_link_libraries(sion PRIVATE OpenSSL::SSL OpenSSL::Crypto)
  find_package(OpenSSL REQUIRED)
else()
  add_definitions(-DSION_DISABLE_SSL)
endif()


set(CMAKE_BUILD_TYPE "Debug")

enable_testing()
add_test (test_run sion -test)
set_tests_properties (test_run
  PROPERTIES PASS_REGULAR_EXPRESSION "TestPassed;All ok")