# OpenThread Unit Tests

This page describes how to build and run OpenThread unit tests. It will be helpful for developers to debug failed unit test cases if they got one in CI or to add some new test cases.

## Build Simulation

The unit tests cannot be built solely without building the whole project. So first build OpenThread on the simulation platform, which will also build all unit tests:

```
# Go to the root directory of OpenThread
$ script/cmake-build simulation
```

## List all tests

To see what tests are available in OpenThread:

```
# Make sure you are at the simulation build directory (build/simulation)
$ ctest -N
```

## Run the Unit Tests

To run all the unit tests:

```
# Make sure you are at the simulation build directory (build/simulation)
$ ctest
```

To run a specific unit test, for example, `ot-test-spinel`:

```
# Make sure you are at the simulation build directory (build/simulation)
$ ctest -R ot-test-spinel
```

## Update a Test Case

If you are developing a unit test case and have made some changes in the test source file, you will need rebuild the test before running it:

```
# Make sure you are at the simulation build directory (build/simulation)
$ ninja <test_name>
```

This will only build the test and take a short time.

If any changes or fixes were made to the OpenThread code, then you'll need to rebuild the entire project:

```
# Make sure you are at the simulation build directory (build/simulation)
$ ninja
```

This will build the updated OpenThread code as well as the test cases.
