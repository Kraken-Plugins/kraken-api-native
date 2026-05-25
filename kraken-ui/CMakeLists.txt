find_package(Java COMPONENTS Runtime Development)

if (Java_FOUND)
    file(GLOB_RECURSE KRAKEN_UI_SOURCES CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/src/main/java/*.java")

    set(KRAKEN_UI_CLASSES_DIR
            "${CMAKE_BINARY_DIR}/kraken-ui/classes")
    set(KRAKEN_UI_STAMP
            "${CMAKE_BINARY_DIR}/kraken-ui/classes.stamp")

    add_custom_command(
            OUTPUT "${KRAKEN_UI_STAMP}"
            COMMAND "${CMAKE_COMMAND}" -E remove_directory
                    "${KRAKEN_UI_CLASSES_DIR}"
            COMMAND "${CMAKE_COMMAND}" -E make_directory
                    "${KRAKEN_UI_CLASSES_DIR}"
            COMMAND "${Java_JAVAC_EXECUTABLE}"
                    --release 11
                    -encoding UTF-8
                    -d "${KRAKEN_UI_CLASSES_DIR}"
                    ${KRAKEN_UI_SOURCES}
            COMMAND "${CMAKE_COMMAND}" -E touch "${KRAKEN_UI_STAMP}"
            DEPENDS ${KRAKEN_UI_SOURCES}
            VERBATIM)

    add_custom_target(kraken-ui-classes
            DEPENDS "${KRAKEN_UI_STAMP}")

    set(KRAKEN_UI_RUN_DEPENDENCIES kraken-ui-classes launcher)
    set(KRAKEN_UI_NATIVE_ARGUMENT)
    if (TARGET kraken-ui-native)
        list(APPEND KRAKEN_UI_RUN_DEPENDENCIES kraken-ui-native)
        set(KRAKEN_UI_NATIVE_ARGUMENT
                "-Dkraken.ui.native=$<TARGET_FILE:kraken-ui-native>")
    endif ()

    add_custom_target(run-kraken-ui
            COMMAND "${Java_JAVA_EXECUTABLE}"
                    ${KRAKEN_UI_NATIVE_ARGUMENT}
                    -cp "${KRAKEN_UI_CLASSES_DIR}"
                    com.kraken.ui.KrakenLauncherApp
            DEPENDS ${KRAKEN_UI_RUN_DEPENDENCIES}
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
            USES_TERMINAL
            VERBATIM)
else ()
    message(STATUS "Java runtime/development tools not found; Kraken UI CMake targets are disabled.")
endif ()
