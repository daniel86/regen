
add_executable(spheres-test spheres-test.cpp)
target_link_libraries(spheres-test factory qt-application ${PROJECT_NAME} ${QT_LIBRARIES})
