
set(QT_APPLIBS qt-application ${QT_LIBRARIES} ${ENGINE_LIBS})

add_library( factory STATIC factory.cpp )

add_executable(assimp-test assimp-test.cpp)
target_link_libraries( assimp-test factory ${QT_APPLIBS} )

add_executable(transparency-test transparency-test.cpp)
target_link_libraries( transparency-test factory ${QT_APPLIBS} )

add_executable(volume-test volume-test.cpp factory.cpp)
target_link_libraries( volume-test factory ${QT_APPLIBS} )

add_executable(instancing-test instancing-test.cpp)
target_link_libraries( instancing-test factory ${QT_APPLIBS} )

add_executable(hdr-test hdr-test.cpp)
target_link_libraries( hdr-test factory ${QT_APPLIBS} )
