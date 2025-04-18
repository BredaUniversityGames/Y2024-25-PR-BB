# THIS DIRECTORY IS ONLY ADDED IF TESTS ARE ENABLED IN CMAKE CONFIG
message(STATUS "### Unit Tests are enabled")

add_executable(UnitTests)
target_output_dir(UnitTests ${CMAKE_BINARY_DIR})

# Tests should be placed under a module "tests" folder,
# but it might be useful to have global tests here
file(GLOB_RECURSE root_tests CONFIGURE_DEPENDS "*.cpp")
target_sources(UnitTests PRIVATE ${root_tests})

# Google Test
FetchContent_Declare(
        GoogleTest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.16.0
        GIT_SHALLOW TRUE
        GIT_PROGRESS TRUE
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
override_option(gtest_force_shared_crt ON)
FetchContent_MakeAvailable(GoogleTest)

target_link_libraries(UnitTests
        PRIVATE ProjectSettings
        PRIVATE GTest::gtest
        PRIVATE GTest::gtest_main
)