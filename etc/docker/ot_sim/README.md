
## Starting simulator

To start the OpenThread simulator, run:
```
docker run --rm -d --name otsim openthread/sim tail -F /dev/null
```
or
```
./start_sim
```
This runs in background a docker container with environment setup to simulate OpenThread nodes.

## Adding a Thread node

To start simulating an OpenThread node #1, run:
```
docker exec -it otsim node 1
```
or
```
./add_node 1
```
This runs a program called node, which is an OpenThread FTD binary, inside the docker container's simulator environment.

## Stopping simulator

To stop the OpenThread simualtor, run:
```
docker stop otsim
```
or
```
./stop_sim
```
This stop the docker daemon process.
