
add_executable(fluid-test
    fluid-test.cpp
    fluid.cpp
    fluid-buffer.cpp
    fluid-operation.cpp
    fluid-parser.cpp
    )
target_link_libraries( fluid-test glut-application ${OGLE_LIBS} )
