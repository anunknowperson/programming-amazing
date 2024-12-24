include(FetchContent)
include(GoogleTest)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
)

FetchContent_MakeAvailable(googletest)
add_library(GTest::GTest INTERFACE IMPORTED
        ../src/core/CameraController.cpp)
target_link_libraries(GTest::GTest INTERFACE gtest_main)

# keeps CACHE cleaner
mark_as_advanced(
        BUILD_GMOCK BUILD_GTEST BUILD_SHARED_LIBS
        gmock_build_tests gtest_build_samples gtest_build_tests
        gtest_disable_pthreads gtest_force_shared_crt gtest_hide_internal_symbols
)

macro(add_gtest TESTNAME)
    # create an executable in which the tests will be stored
    add_executable(${TESTNAME} ${ARGN})
    # link the Google test infrastructure, mocking library, and a default main function to
    # the test executable.  Remove gtest_main if writing your own main function.
    target_link_libraries(${TESTNAME} ${PROJECT_NAME} gtest gmock gtest_main)
    # gtest_discover_tests replaces gtest_add_tests,
    # see https://cmake.org/cmake/help/v3.10/module/GoogleTest.html for more options to pass to it
    gtest_discover_tests(${TESTNAME}
            # set a working directory so your project root so that you can find test data via paths relative to the project root
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
    set_target_properties(${TESTNAME} PROPERTIES FOLDER tests)
endmacro()