add_executable(launcher
        dll_injector.cpp
        dll_injector.hpp
        main.cpp
        plugin_stager.cpp
        plugin_stager.hpp
        process_launcher.cpp
        process_launcher.hpp
        win32_handle.hpp
)

target_compile_definitions(launcher PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(launcher PRIVATE user32)

add_dependencies(launcher plugin-core)
