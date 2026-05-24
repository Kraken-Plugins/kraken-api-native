include(FetchContent)

FetchContent_Declare(
        minhook
        GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
        GIT_TAG        v1.3.4
)
FetchContent_MakeAvailable(minhook)

add_library(plugin-core SHARED
        dllmain.cpp
        hooks.cpp
        hooks.hpp
        logger.cpp
        logger.hpp
        offsets.hpp
        patcher.cpp
        patcher.hpp
)

target_compile_definitions(plugin-core PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_include_directories(plugin-core PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(plugin-core PRIVATE minhook)
