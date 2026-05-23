add_executable(launcher main.cpp)

# After building, copy the DLL next to the launcher exe
add_custom_command(TARGET launcher POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:plugin-core>   # plugin-core.dll output path
        $<TARGET_FILE_DIR:launcher>  # launcher.exe output dir
        COMMENT "Copying plugin-core.dll next to launcher..."
)

add_dependencies(launcher plugin-core)
target_link_libraries(launcher PRIVATE)
