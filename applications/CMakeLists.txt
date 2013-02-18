

set(USE_FLTK_TEST_APPLICATIONS 1)
set(USE_FLTK_FLUID_APPLICATION 1)

configure_file( application-config.h.cmake application-config.h )

add_library( glut-application STATIC
       ogle-application.cpp
       glut-ogle-application.cpp
       )

##################

FIND_PACKAGE(Qt4 COMPONENTS QtCore QtGui QtOpenGL REQUIRED)

INCLUDE(${QT_USE_FILE})
ADD_DEFINITIONS(${QT_DEFINITIONS})

SET(QT_OGLE_HEADERS qt-gl-widget.h)
QT4_WRAP_CPP(QT_OGLE_HEADERS_MOC ${QT_OGLE_HEADERS})

add_library( qt-application STATIC
       ogle-application.cpp
       qt-gl-widget.cpp
       qt-ogle-application.cpp
       ${QT_OGLE_HEADERS_MOC}
       )

##################

# find_package(FLTK REQUIRED)
execute_process(COMMAND fltk-config --ldflags
    OUTPUT_VARIABLE FLTK_LIBRARIES OUTPUT_STRIP_TRAILING_WHITESPACE)
set(FLTK_LIBRARIES ${FLTK_LIBRARIES} -lfltk_images -lfltk_gl)

add_library( fltk-application STATIC
       ogle-application.cpp
       fltk-ogle-application.cpp
       )

add_subdirectory(test)
add_subdirectory(texture-update-editor)
add_subdirectory(video-player)
       