
add_library(factory STATIC factory.cpp )

add_executable(assimp-test assimp-test.cpp)
target_link_libraries(assimp-test
    factory qt-application ${PROJECT_NAME} ${QT_LIBRARIES})

add_executable(transparency-test transparency-test.cpp)
target_link_libraries(transparency-test
    factory qt-application ${PROJECT_NAME} ${QT_LIBRARIES})

add_executable(volume-test volume-test.cpp)
target_link_libraries(volume-test
    factory qt-application ${PROJECT_NAME} ${QT_LIBRARIES})

add_executable(instancing-test instancing-test.cpp)
target_link_libraries(instancing-test
    factory qt-application ${PROJECT_NAME} ${QT_LIBRARIES})

add_executable(hdr-test hdr-test.cpp)
target_link_libraries(hdr-test
    factory qt-application ${PROJECT_NAME} ${QT_LIBRARIES})

if(WIN32)
    # collect dll files
    foreach(LIB_FILE ${REGEN_LIBRARIES} ${QT_LIBRARIES})
        get_filename_component(LIB_FILE_NAME ${LIB_FILE} NAME_WE)
        get_filename_component(LIB_PATH "${LIB_FILE}" PATH)
        string(REPLACE ".lib" ".dll" DLL_FILE "${LIB_FILE}")
        
        if(NOT "${LIB_FILE_NAME}" STREQUAL "${LIB_FILE}" AND
           NOT "${LIB_FILE}" STREQUAL "${DLL_FILE}")
            message(INFO "    lib file: ${LIB_FILE}")
            if(EXISTS ${DLL_FILE})
                message(INFO "    dll file: ${DLL_FILE}")
                set(DLL_FILES ${DLL_FILES} ${DLL_FILE})
            else()
                get_filename_component(LIB_PARENT_PATH "${LIB_PATH}" PATH)
                set(DLL_FILE "${LIB_PARENT_PATH}/bin/${LIB_FILE_NAME}.dll")
                if(EXISTS ${DLL_FILE})
                    message(INFO "    dll file: ${DLL_FILE}")
                    set(DLL_FILES ${DLL_FILES} ${DLL_FILE})
                endif()
            endif()
        endif()
    endforeach()
    
    # find the build directory
    get_filename_component(BIN_DIR ${CMAKE_CURRENT_BINARY_DIR} REALPATH)
    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set(BUILD_DIR "${BIN_DIR}/Debug/")
    else()
        set(BUILD_DIR "${BIN_DIR}/Release/")
    endif()
    
    # and add post build command to copy dll files to build directory
    foreach(DLL_FILE ${DLL_FILES})
        message("add_custom_command copy ${DLL_FILE} ${BUILD_DIR}")
        add_custom_command(
            COMMAND ${CMAKE_COMMAND} -E copy ${DLL_FILE} ${BUILD_DIR}
        )
    endforeach()
endif()
