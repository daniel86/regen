
set(USE_QT_TEST_APPLICATIONS 0)
set(USE_QT_FLUID_APPLICATION 0)

configure_file( application-config.h.cmake application-config.h )

add_library( glut-application STATIC
       test-render-tree.cpp
       test-camera-manipulator.cpp
       ogle-application.cpp
       glut-ogle-application.cpp
       )

##################

find_package(FLTK REQUIRED)

add_library( fltk-application STATIC
       test-render-tree.cpp
       test-camera-manipulator.cpp
       ogle-application.cpp
       fltk-ogle-application.cpp
       )
