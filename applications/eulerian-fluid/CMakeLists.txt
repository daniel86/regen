
add_executable(fluid-test
    fluid.cpp
    fluid-buffer.cpp
    fluid-operation.cpp
    fluid-animation.cpp
    fluid-parser.cpp
    fluid-test.cpp
    )
target_link_libraries( fluid-test glut-application ${OGLE_LIBS} )
