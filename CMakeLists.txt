cmake_minimum_required(VERSION 4.0.0)
project(diddytron VERSION 0.1.0 LANGUAGES C)

set(CMAKE_MACOSX_RPATH ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#find_package(Vulkan REQUIRED)

include(FetchContent)
FetchContent_Declare(
    volk
    GIT_REPOSITORY https://github.com/zeux/volk.git
    GIT_TAG master
)
FetchContent_MakeAvailable(volk)



add_executable(diddytron src/main.c)

set_target_properties(diddytron PROPERTIES
    BUILD_RPATH "/usr/local/lib"
    INSTALL_RPATH "/usr/local/lib"
    COMPILE_FLAGS "-Wall"
)

target_link_libraries(diddytron PRIVATE volk::volk)




