
set(GLUT_APPLIBS glut-application ${GLUT_LIBRARIES} ${OGLE_LIBS})
set(QT_APPLIBS qt-application ${QT_LIBRARIES} ${OGLE_LIBS})

add_executable(fluid-test
    fluid.cpp
    fluid-buffer.cpp
    fluid-operation.cpp
    fluid-animation.cpp
    fluid-parser.cpp
    fluid-test.cpp
    )
target_link_libraries( fluid-test ${GLUT_APPLIBS} )
