
set(APPLIBS fltk-application ${FLTK_LIBRARIES} ${OGLE_LIBS})

add_executable(ogle-texture-update-editor
    texture-update-editor.cpp
    )
target_link_libraries( ogle-texture-update-editor ${APPLIBS} )
