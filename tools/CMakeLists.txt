include_directories("${PROJECT_SOURCE_DIR}/src")

add_executable(jp2cli jp2cli.c)
target_link_libraries(jp2cli jp2library)

add_executable(jp2dump jp2dump.c)
target_link_libraries(jp2dump jp2library)

add_executable(jp2segments jp2segments.c)
