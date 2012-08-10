
add_definitions( -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS )

set(SOURCES 
       av/audio.cpp
       av/audio-buffer.cpp
       av/audio-source.cpp
       av/audio-stream.cpp
       av/demuxer.cpp
       av/ffmpeg-stream.cpp
       av/video-stream.cpp
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
    DESTINATION ${HEADER_INSTALL_PATH}/av
    )
