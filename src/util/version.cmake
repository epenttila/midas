execute_process(
    COMMAND ${GIT_EXECUTABLE} describe --always --dirty --long --tags
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE)

configure_file("${INPUT}" "${OUTPUT}" @ONLY)
