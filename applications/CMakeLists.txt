

set(USE_FLTK_TEST_APPLICATIONS 1)
set(USE_FLTK_FLUID_APPLICATION 1)

configure_file( application-config.h.cmake application-config.h )

add_library( glut-application STATIC
       test-render-tree.cpp
       test-camera-manipulator.cpp
       ogle-application.cpp
       glut-ogle-application.cpp
       )

##################

# find_package(FLTK REQUIRED)
execute_process(COMMAND fltk-config --ldflags
    OUTPUT_VARIABLE FLTK_LIBRARIES OUTPUT_STRIP_TRAILING_WHITESPACE)
set(FLTK_LIBRARIES ${FLTK_LIBRARIES} -lfltk_images -lfltk_gl)

add_library( fltk-application STATIC
       test-render-tree.cpp
       test-camera-manipulator.cpp
       ogle-application.cpp
       fltk-ogle-application.cpp
       )

add_subdirectory(test)
add_subdirectory(texture-update-editor)
       