include_directories("${PROJECT_SOURCE_DIR}/src")

add_executable(test_001 test_001.c common.c)
target_link_libraries(test_001 jp2library)

enable_testing()

add_test(test_001 test_001)
