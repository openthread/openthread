# OpenThread Unit Tests

This page shows how to build and run OpenThread unit tests. It will be helpful for developers to debug failed unit test cases if they got one in CI or to add some new test cases.

## Build Simulation

The unit tests cannot be built solely without building the whole project. So first build OpenThread on simulation platform:

```
# Go to the root directory of OpenThread
./script/cmake-build simulation
```

Now all the unit tests should have been built.

## List all tests

To see what tests are there in OpenThread:

```
# Make sure you are at the simulation build directory (build/simulation)
ctest -N
```

## Run the Unit Tests

To run all the unit tests:

```
# Make sure you are at the simulation build directory (build/simulation)
ctest
```

To run a specific unit test, for example, `ot-test-spinel`:

```
# Make sure you are at the simulation build directory (build/simulation)
ctest -R ot-test-spinel
```

## Update A Test Case

For example, we are developing a unit test case. We made some changes in the test source file and want to run the updated executable. We can run:

```
# Make sure you are at the simulation build directory (build/simulation)
ninja <test_name>
```

This will only build the test and take a short time.

If we found there are some errors in OpenThread code and we made some fixes, then we need to build the whole project. Simply run:

```
# Make sure you are at the simulation build directory (build/simulation)
ninja
```

This will build the updated OpenThread code as well as the test cases.
