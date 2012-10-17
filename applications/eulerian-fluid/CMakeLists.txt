
IF( USE_FLTK_TEST_APPLICATIONS )
    set(APPLIBS fltk-application ${FLTK_LIBRARIES} ${OGLE_LIBS})
ELSE( USE_FLTK_TEST_APPLICATIONS )
    set(APPLIBS glut-application ${GLUT_LIBRARIES} ${OGLE_LIBS})
ENDIF( USE_FLTK_TEST_APPLICATIONS )

add_executable(ogle-fluid-editor
    fluid.cpp
    fluid-buffer.cpp
    fluid-operation.cpp
    fluid-animation.cpp
    fluid-parser.cpp
    fluid-editor.cpp
    )
target_link_libraries( ogle-fluid-editor ${APPLIBS} )
