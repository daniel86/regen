
IF( USE_FLTK_TEST_APPLICATIONS )
    set(APPLIBS fltk-application ${FLTK_LIBRARIES} ${OGLE_LIBS})
ELSE( USE_FLTK_TEST_APPLICATIONS )
    set(APPLIBS glut-application ${GLUT_LIBRARIES} ${OGLE_LIBS})
ENDIF( USE_FLTK_TEST_APPLICATIONS )

add_executable(transparency-test transparency-test.cpp)
target_link_libraries( transparency-test ${APPLIBS} )

add_executable(normal-map-test normal-map-test.cpp)
target_link_libraries( normal-map-test ${APPLIBS} )

add_executable(assimp-test assimp-test.cpp)
target_link_libraries( assimp-test ${APPLIBS} )

add_executable(volume-test volume-test.cpp)
target_link_libraries( volume-test ${APPLIBS} )

add_executable(instancing-test instancing-test.cpp)
target_link_libraries( instancing-test ${APPLIBS} )

add_executable(video-test video-test.cpp)
target_link_libraries( video-test ${APPLIBS} )

add_executable(mesh-animation-test mesh-animation-test.cpp)
target_link_libraries( mesh-animation-test ${APPLIBS} )

add_executable(hdr-test hdr-test.cpp)
target_link_libraries( hdr-test ${APPLIBS} )

add_executable(particles-test particles-test.cpp)
target_link_libraries( particles-test ${APPLIBS} )

add_executable(depth-of-field-test depth-of-field-test.cpp)
target_link_libraries( depth-of-field-test ${APPLIBS} )

add_executable(motion-blur-test motion-blur-test.cpp)
target_link_libraries( motion-blur-test ${APPLIBS} )
