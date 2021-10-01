# OpenThread Border Router Tests

## Run Border Router (BR) tests locally

BR tests run in isolated Docker network and containers, so a new OTBR Docker image needs to be created before running these tests:

```shell
# Use root privilege when necessary.

# Download OpenThread's branch of wireshark. Run this for the first time.
./script/test get_thread_wireshark

# Clear current OpenThread directory (remember to add new source files).
git clean -xfd

# Rebuild the OTBR Docker image if OTBR source code is updated.
LOCAL_OTBR_DIR=$HOME/ot-br-posix ./script/test build_otbr_docker

# Build simulated OpenThread firmware.
VIRTUAL_TIME=0 ./script/test build

# Run the BR tests locally.
TEST_CASE=./tests/scripts/thread-cert/border_router/test_advertising_proxy.py
VERBOSE=1 PACKET_VERIFICATION=1 VIRTUAL_TIME=0 ./script/test cert_suite ${TEST_CASE}
```
