
find_package(GLUT)
find_package(OpenGL)

find_library(IL_LIB IL)

add_executable(unittests
               test.cpp
               test-events.cpp
               test-algebra.cpp)
target_link_libraries (unittests
    ${CMAKE_BINARY_DIR}/src/libogle.so
    ${GLUT_LIBRARY}
    ${OPENGL_LIBRARY}
    ${IL_LIB}
    -lGLEW
    -lcppunit
    )
