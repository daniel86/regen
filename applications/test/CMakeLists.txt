
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

