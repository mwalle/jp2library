include_directories("${PROJECT_SOURCE_DIR}/src")
include_directories(${JNI_INCLUDE_DIRS})

add_library(jp12serial SHARED jp12serial_compat.c)
target_link_libraries(jp12serial jp2library)
