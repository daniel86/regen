
execute_process(COMMAND fltk-config --ldflags
    OUTPUT_VARIABLE FLTK_LIBRARIES OUTPUT_STRIP_TRAILING_WHITESPACE)
set(FLTK_LIBRARIES ${FLTK_LIBRARIES} -lfltk_images -lfltk_gl)
add_subdirectory(fltk)
add_subdirectory(texture-update-editor)

IF(BUILD_GLUT_APPLICATION)
    add_subdirectory(glut)
ENDIF(BUILD_GLUT_APPLICATION)

IF(BUILD_TESTS OR BUILD_VIDEO_PLAYER)
    FIND_PACKAGE(Qt4 COMPONENTS QtCore QtGui QtOpenGL REQUIRED)
    INCLUDE(${QT_USE_FILE})
    ADD_DEFINITIONS(${QT_DEFINITIONS})
    add_subdirectory(qt)
ENDIF(BUILD_TESTS OR BUILD_VIDEO_PLAYER)

IF(BUILD_TESTS)
    add_subdirectory(test)
ENDIF(BUILD_TESTS)

IF(BUILD_VIDEO_PLAYER)
    add_subdirectory(video-player)
ENDIF(BUILD_VIDEO_PLAYER)
   