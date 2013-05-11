
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
    if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
        set(BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/Debug/")
    else()
        set(BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/Release/")
    endif()
    string(REPLACE ".lib" ".dll" REGEN_LIBRARIES_DLL "${REGEN_LIBRARIES} ${QT_LIBRARIES}")
    
    foreach(DLL_FILE ${REGEN_LIBRARIES_DLL})
        get_filename_component(DLL_FILE_NAME ${DLL_FILE} NAME_WE)
    
        if(EXISTS ${DLL_FILE} AND NOT "${DLL_FILE_NAME}" STREQUAL "${DLL_FILE}")
            message(INFO "    ${DLL_FILE} will be copied to ${BUILD_DIR}.")
            add_custom_command(
                TARGET regen POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy ${DLL_FILE} ${BUILD_DIR}
            )
        endif()
    endforeach()
endif()
