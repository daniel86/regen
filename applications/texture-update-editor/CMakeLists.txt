
IF( USE_FLTK_TEST_APPLICATIONS )
    set(APPLIBS fltk-application ${FLTK_LIBRARIES} ${OGLE_LIBS})
ELSE( USE_FLTK_TEST_APPLICATIONS )
    set(APPLIBS glut-application ${GLUT_LIBRARIES} ${OGLE_LIBS})
ENDIF( USE_FLTK_TEST_APPLICATIONS )

add_executable(ogle-texture-update-editor
    texture-update-editor.cpp
    )
target_link_libraries( ogle-texture-update-editor ${APPLIBS} )
