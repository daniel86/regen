
add_library( algebra STATIC
       vector.cpp
       frustum.cpp
       )
       
install(
    FILES
        frustum.h
        matrix.h
        quaternion.h
        vector.h
    DESTINATION include/ogle/algebra
    )
