
## Running wpantund from a Docker container:

For a device that has a Thread radio attached to port /dev/ttyUSB0, start wpantund as follows:

```
docker run --rm --detach -e "OPENTHREAD_DEVICE_PORT=/dev/ttyUSB0" --cap-add=NET_ADMIN --device=/dev/ttyUSB0 --name=wpantund openthread/wpantund
```

Once wpantund is running, one can control the Thread interface with wpanctl as follows:

```
docker exec -it wpantund wpanctl
```


## Content

arm32v7_ubuntu_wpantund
- wpantund running on ARMv7 (e.g. Raspberry Pi)

x86_ubuntu_wpantund
- wpantund running on x86

ot_sim
- OpenThread Posix simulator

