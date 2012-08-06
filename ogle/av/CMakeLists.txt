
add_definitions( -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS )

add_library( av STATIC
       audio.cpp
       audio-buffer.cpp
       audio-source.cpp
       audio-stream.cpp
       demuxer.cpp
       ffmpeg-stream.cpp
       video-stream.cpp
       )

       
install(
    FILES
       audio.h
       audio-buffer.h
       audio-source.h
       audio-stream.h
       demuxer.h
       ffmpeg-stream.h
       video-stream.h
    DESTINATION include/ogle/av
    )
