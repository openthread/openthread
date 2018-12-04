
## Running wpantund from a Docker container:

For a device that has a Thread radio attached to port `/dev/ttyUSB0`, start `wpantund` as follows:

```
docker run --rm --detach -e "OPENTHREAD_DEVICE_PORT=/dev/ttyUSB0" --cap-add=NET_ADMIN --device=/dev/ttyUSB0 --name=wpantund openthread/wpantund
```

Once `wpantund` is running, one can control the Thread interface with `wpanctl` as follows:

```
docker exec -it wpantund wpanctl
```


## Content

arm32v7_ubuntu_wpantund
- `wpantund` running on ARMv7 (e.g. Raspberry Pi)

x86_ubuntu_wpantund
- `wpantund` running on x86

ot_sim
- OpenThread POSIX simulator

codelab_otsim
- For use with the [Docker Simulation Codelab](https://codelabs.developers.google.com/codelabs/openthread-simulation/), contains the OpenThread POSIX example and `wpantund` pre-built and ready to use.

environment
- Development environment with the GNU toolchain and all required OpenThread dependencies installed. OpenThread is not built in this image.

Images built from these Dockerfiles are available to pull from [Docker Hub](https://hub.docker.com/u/openthread/). See [Docker Support on openthread.io](https://openthread.io/guides#docker_support) for more information.