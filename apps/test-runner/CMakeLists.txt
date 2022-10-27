set(APP_NAME TestRunner)

file(GLOB_RECURSE TEST_SRCS "tests/*.cpp")

add_executable(${APP_NAME} "app.cpp" ${TEST_SRCS})
target_link_libraries(${APP_NAME} GraphiT)
