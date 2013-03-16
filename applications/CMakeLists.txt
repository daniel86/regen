
add_subdirectory(glut)

execute_process(COMMAND fltk-config --ldflags
    OUTPUT_VARIABLE FLTK_LIBRARIES OUTPUT_STRIP_TRAILING_WHITESPACE)
set(FLTK_LIBRARIES ${FLTK_LIBRARIES} -lfltk_images -lfltk_gl)
add_subdirectory(fltk)

FIND_PACKAGE(Qt4 COMPONENTS QtCore QtGui QtOpenGL REQUIRED)
INCLUDE(${QT_USE_FILE})
ADD_DEFINITIONS(${QT_DEFINITIONS})
add_subdirectory(qt)

add_subdirectory(test)
add_subdirectory(texture-update-editor)
add_subdirectory(video-player)
       