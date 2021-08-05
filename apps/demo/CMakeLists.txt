set(APP_NAME Demo)

add_executable(${APP_NAME} "app.cpp")
target_link_libraries(${APP_NAME} GraphiT)

file(GLOB ASSETS "${CMAKE_CURRENT_SOURCE_DIR}/assets/*")
add_custom_command(
    TARGET ${APP_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy ${ASSETS} "${CMAKE_BINARY_DIR}/assets"
    DEPENDS ${ASSETS}
)
add_custom_target(${APP_NAME}_COPY_ASSETS DEPENDS ${ASSETS})
add_dependencies(${APP_NAME} ${APP_NAME}_COPY_ASSETS)
