

IF( USE_QT_FLUID_APPLICATION )
    set(APPLIBS qt-application ${QT_LIBRARIES} ${OGLE_LIBS})
ELSE( USE_QT_FLUID_APPLICATION )
    set(APPLIBS glut-application ${GLUT_LIBRARIES} ${OGLE_LIBS})
ENDIF( USE_QT_FLUID_APPLICATION )

add_executable(fluid-test
    fluid.cpp
    fluid-buffer.cpp
    fluid-operation.cpp
    fluid-animation.cpp
    fluid-parser.cpp
    fluid-test.cpp
    )
target_link_libraries( fluid-test ${APPLIBS} )
