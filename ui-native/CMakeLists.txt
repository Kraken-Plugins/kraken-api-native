find_package(JNI COMPONENTS AWT JVM)

if (JNI_FOUND AND JAVA_AWT_LIBRARY)
    add_library(kraken-ui-native SHARED
            ui_native.cpp
    )

    target_compile_definitions(kraken-ui-native PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)
    target_include_directories(kraken-ui-native PRIVATE ${JNI_INCLUDE_DIRS})
    target_link_libraries(kraken-ui-native PRIVATE ${JNI_LIBRARIES} ${JAVA_AWT_LIBRARY} user32)
else ()
    message(STATUS "JNI AWT library not found; kraken-ui-native target is disabled.")
endif ()
