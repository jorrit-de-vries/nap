if(NOT TARGET asio)
    find_package(asio REQUIRED)
endif()

if(NAP_BUILD_CONTEXT MATCHES "source")
    target_include_directories(${PROJECT_NAME} PUBLIC src ${ASIO_INCLUDE_DIR})
    target_compile_definitions(${PROJECT_NAME} PUBLIC ASIO_STANDALONE)

    # additional definitions
    if(WIN32)
        target_compile_definitions(${PROJECT_NAME} PUBLIC WIN32_LEAN_AND_MEAN _WIN32_WINNT=0x0A00)
    endif()

    # Package asio into platform release
    install(FILES ${ASIO_DIST_FILES} DESTINATION thirdparty/asio)
    install(DIRECTORY ${ASIO_INCLUDE_DIR} DESTINATION thirdparty/asio)
else()
    add_include_to_interface_target(napasio ${ASIO_INCLUDE_DIRS})
    add_define_to_interface_target(napasio ASIO_STANDALONE)

    if(WIN32)
        # Define _WIN32_WINNT for ASIO
        add_define_to_interface_target(napasio WIN32_LEAN_AND_MEAN)
        add_define_to_interface_target(napasio _WIN32_WINNT=0x0A00)
    endif()

    # Install asio license into packaged project
    install(FILES ${THIRDPARTY_DIR}/asio/LICENSE_1_0.txt DESTINATION licenses/asio)
endif()
