
add_executable(hello-world hello-world.cpp)
target_link_libraries( hello-world glut-application ${OGLE_LIBS} )

add_executable(render-to-texture render-to-texture.cpp)
target_link_libraries( render-to-texture glut-application ${OGLE_LIBS} )

add_executable(normal-map normal-map.cpp)
target_link_libraries( normal-map glut-application ${OGLE_LIBS} )

add_executable(transform-feedback transform-feedback.cpp)
target_link_libraries( transform-feedback glut-application ${OGLE_LIBS} )

add_executable(video-test video-test.cpp)
target_link_libraries( video-test glut-application ${OGLE_LIBS} )
