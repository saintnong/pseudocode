# =========================================================
# Grabs the current git version and embeds into version.hpp
# -> Automatically run during compile time
# =========================================================

execute_process(
    COMMAND git describe --tags --always
    WORKING_DIRECTORY "${SOURCE_DIR}"
    OUTPUT_VARIABLE GIT_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

configure_file(
    "${SOURCE_DIR}/src/version.hpp.in"
    "${BINARY_DIR}/generated/version.hpp"
    @ONLY
)