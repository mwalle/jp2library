cmake_minimum_required(VERSION 2.8)

project(jp2library)

set(CMAKE_C_FLAGS "-Wall -Werror")

find_package(JNI)

if (JNI_FOUND)
	add_subdirectory(jni)
else()
	message (STATUS "JDK not found. Skipping building of the JNI library.")
	message (STATUS "Try to set the environment variable JAVA_HOME.")
endif()

add_subdirectory(src)
add_subdirectory(tools)
add_subdirectory(tests)
