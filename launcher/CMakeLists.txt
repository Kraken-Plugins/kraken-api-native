add_executable(launcher
        main.cpp
        dll_injector.cpp
        dll_injector.hpp
        process_launcher.cpp
        process_launcher.hpp
        win32_handle.hpp
)

target_compile_definitions(launcher PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

add_dependencies(launcher plugin-core)

add_custom_command(TARGET launcher POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:plugin-core>
        $<TARGET_FILE_DIR:launcher>
        COMMENT "Copying plugin-core.dll next to launcher..."
)
