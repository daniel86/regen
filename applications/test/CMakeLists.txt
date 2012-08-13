
add_executable(hello-world hello-world.cpp)
target_link_libraries( hello-world glut-application ${OGLE_LIBS} )

add_executable(render-to-texture-test render-to-texture-test.cpp)
target_link_libraries( render-to-texture-test glut-application ${OGLE_LIBS} )

add_executable(normal-map-test normal-map-test.cpp)
target_link_libraries( normal-map-test glut-application ${OGLE_LIBS} )

add_executable(assimp-test assimp-test.cpp)
target_link_libraries( assimp-test glut-application ${OGLE_LIBS} )

add_executable(terrain-test terrain-test.cpp)
target_link_libraries( terrain-test glut-application ${OGLE_LIBS} )

add_executable(volume-test volume-test.cpp)
target_link_libraries( volume-test glut-application ${OGLE_LIBS} )

add_executable(instancing-test instancing-test.cpp)
target_link_libraries( instancing-test glut-application ${OGLE_LIBS} )

add_executable(transform-feedback-test transform-feedback-test.cpp)
target_link_libraries( transform-feedback-test glut-application ${OGLE_LIBS} )

add_executable(video-test video-test.cpp)
target_link_libraries( video-test glut-application ${OGLE_LIBS} )

add_executable(hdr-test hdr-test.cpp)
target_link_libraries( hdr-test glut-application ${OGLE_LIBS} )

add_executable(anti-aliasing-test anti-aliasing-test.cpp)
target_link_libraries( anti-aliasing-test glut-application ${OGLE_LIBS} )

add_executable(motion-blur-test motion-blur-test.cpp)
target_link_libraries( motion-blur-test glut-application ${OGLE_LIBS} )
