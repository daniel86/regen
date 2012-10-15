
set(GLUT_APPLIBS glut-application ${GLUT_LIBRARIES} ${OGLE_LIBS})
set(QT_APPLIBS qt-application ${QT_LIBRARIES} ${OGLE_LIBS})

add_executable(hello-world hello-world.cpp)
target_link_libraries( hello-world ${QT_APPLIBS} )

add_executable(render-to-texture-test render-to-texture-test.cpp)
target_link_libraries( render-to-texture-test ${QT_APPLIBS} )

add_executable(normal-map-test normal-map-test.cpp)
target_link_libraries( normal-map-test ${QT_APPLIBS} )

add_executable(assimp-test assimp-test.cpp)
target_link_libraries( assimp-test ${QT_APPLIBS} )

add_executable(volume-test volume-test.cpp)
target_link_libraries( volume-test ${QT_APPLIBS} )

add_executable(instancing-test instancing-test.cpp)
target_link_libraries( instancing-test ${QT_APPLIBS} )

add_executable(transform-feedback-test transform-feedback-test.cpp)
target_link_libraries( transform-feedback-test ${QT_APPLIBS} )

add_executable(video-test video-test.cpp)
target_link_libraries( video-test ${QT_APPLIBS} )

add_executable(mesh-animation-test mesh-animation-test.cpp)
target_link_libraries( mesh-animation-test ${QT_APPLIBS} )

add_executable(hdr-test hdr-test.cpp)
target_link_libraries( hdr-test ${QT_APPLIBS} )

add_executable(anti-aliasing-test anti-aliasing-test.cpp)
target_link_libraries( anti-aliasing-test ${QT_APPLIBS} )
