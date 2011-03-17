
find_package(OpenGL)

find_library(IL_LIB IL)
#find_library(FTGL_LIB FTGL)

add_executable(unittests
               test.cpp
               #test-events.cpp
               test-algebra.cpp)
target_link_libraries (unittests
    ${CMAKE_BINARY_DIR}/src/libogle.so
    ${OPENGL_LIBRARY}
    ${IL_LIB}
    -lftgl
    -lGLEW
    -lcppunit
    )
