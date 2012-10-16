
IF( USE_QT_FLUID_APPLICATION )
    set(APPLIBS qt-application ${QT_LIBRARIES} ${OGLE_LIBS})
ELSE( USE_QT_FLUID_APPLICATION )
    set(APPLIBS glut-application ${GLUT_LIBRARIES} ${OGLE_LIBS})
ENDIF( USE_QT_FLUID_APPLICATION )

add_executable(hello-world hello-world.cpp)
target_link_libraries( hello-world ${APPLIBS} )

add_executable(render-to-texture-test render-to-texture-test.cpp)
target_link_libraries( render-to-texture-test ${APPLIBS} )

add_executable(normal-map-test normal-map-test.cpp)
target_link_libraries( normal-map-test ${APPLIBS} )

add_executable(assimp-test assimp-test.cpp)
target_link_libraries( assimp-test ${APPLIBS} )

add_executable(volume-test volume-test.cpp)
target_link_libraries( volume-test ${APPLIBS} )

add_executable(instancing-test instancing-test.cpp)
target_link_libraries( instancing-test ${APPLIBS} )

add_executable(transform-feedback-test transform-feedback-test.cpp)
target_link_libraries( transform-feedback-test ${APPLIBS} )

add_executable(video-test video-test.cpp)
target_link_libraries( video-test ${APPLIBS} )

add_executable(mesh-animation-test mesh-animation-test.cpp)
target_link_libraries( mesh-animation-test ${APPLIBS} )

add_executable(hdr-test hdr-test.cpp)
target_link_libraries( hdr-test ${APPLIBS} )

add_executable(anti-aliasing-test anti-aliasing-test.cpp)
target_link_libraries( anti-aliasing-test ${APPLIBS} )
