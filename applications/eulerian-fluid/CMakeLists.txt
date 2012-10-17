
IF( USE_FLTK_TEST_APPLICATIONS )
    set(APPLIBS fltk-application ${FLTK_LIBRARIES} ${OGLE_LIBS})
ELSE( USE_FLTK_TEST_APPLICATIONS )
    set(APPLIBS glut-application ${GLUT_LIBRARIES} ${OGLE_LIBS})
ENDIF( USE_FLTK_TEST_APPLICATIONS )

add_executable(fluid-test
    fluid.cpp
    fluid-buffer.cpp
    fluid-operation.cpp
    fluid-animation.cpp
    fluid-parser.cpp
    fluid-test.cpp
    )
target_link_libraries( fluid-test ${APPLIBS} )
