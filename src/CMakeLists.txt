# CMake 最低版本号要求
cmake_minimum_required (VERSION 3.5)

set(CMAKE_CXX_STANDARD 17)
aux_source_directory(. DIR_SRCS)

#

if(DEFINED VCPKG_PATH)
  message("enable ssl")
  set(CMAKE_TOOLCHAIN_FILE ${VCPKG_PATH}/scripts/buildsystems/vcpkg.cmake)
  project (sion)
else()
  message("disable ssl")
  project (sion)
endif()

add_executable(sion ${DIR_SRCS})
target_link_libraries(sion PRIVATE pthread)

if(DEFINED VCPKG_PATH)
  target_link_libraries(sion PRIVATE OpenSSL::SSL OpenSSL::Crypto)
  find_package(OpenSSL REQUIRED)
else()
  add_definitions(-DSION_DISABLE_SSL)
endif()


set(CMAKE_BUILD_TYPE "Debug")

