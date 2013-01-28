
set(FLTK_APPLIBS fltk-application ${FLTK_LIBRARIES} ${OGLE_LIBS})
set(QT_APPLIBS qt-application ${QT_LIBRARIES} ${OGLE_LIBS})

add_executable(transparency-test transparency-test.cpp)
target_link_libraries( transparency-test ${FLTK_APPLIBS} )

add_executable(normal-map-test normal-map-test.cpp)
target_link_libraries( normal-map-test ${FLTK_APPLIBS} )

add_executable(assimp-test assimp-test.cpp)
target_link_libraries( assimp-test ${FLTK_APPLIBS} )

add_executable(volume-test volume-test.cpp)
target_link_libraries( volume-test ${FLTK_APPLIBS} )

add_executable(instancing-test instancing-test.cpp)
target_link_libraries( instancing-test ${FLTK_APPLIBS} )

add_executable(mesh-animation-test mesh-animation-test.cpp)
target_link_libraries( mesh-animation-test ${FLTK_APPLIBS} )

add_executable(hdr-test hdr-test.cpp)
target_link_libraries( hdr-test ${FLTK_APPLIBS} )

add_executable(depth-of-field-test depth-of-field-test.cpp)
target_link_libraries( depth-of-field-test ${FLTK_APPLIBS} )

add_executable(motion-blur-test motion-blur-test.cpp)
target_link_libraries( motion-blur-test ${FLTK_APPLIBS} )
