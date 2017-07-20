project(cli)

add_custom_target(
    ${PROJECT_NAME}
    ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR} ${PROJECT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:pyabslib> ${PROJECT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:pycfrlib> ${PROJECT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:pygamelib> ${PROJECT_BINARY_DIR}
    COMMAND ${PYTHON_EXECUTABLE} -m pep8 --max-line-length=1000 --exclude='ext' ${PROJECT_BINARY_DIR}
    COMMAND ${PYTHON_EXECUTABLE} -m pylint ${PROJECT_BINARY_DIR}/midas
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/ DESTINATION . FILES_MATCHING PATTERN "*.py")
install(FILES $<TARGET_FILE:pyabslib> $<TARGET_FILE:pycfrlib> $<TARGET_FILE:pygamelib> DESTINATION .)

add_custom_target(check-${PROJECT_NAME} python3 -m pytest DEPENDS ${PROJECT_NAME})

add_dependencies(check check-${PROJECT_NAME})
