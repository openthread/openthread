# `toranj` test framework

`toranj` is a test framework for OpenThread.

It provides two modes:

- `toranj-cli` which enables testing of OpenThread using its CLI interface.
- `toranj-ncp` which enables testing of the combined behavior of OpenThread (in NCP mode), spinel interface, and `wpantund` driver on linux.

`toranj` features:

- It is developed in Python.
- It can be used to simulate multiple nodes forming complex network topologies.
- It allows testing of network interactions between many nodes (IPv6 traffic exchanges).
- `toranj` in NCP mode runs `wpantund` natively with OpenThread in NCP mode on simulation platform (real-time).
- `toranj` in CLI mode runs `ot-cli-ftd` on simulation platform (real-time).
- `toranj` tests run as part of GitHub Actions pull request validation in OpenThread and `wpantund` GitHub projects.

## `toranj` modes

- [`toranj-cli` guide](README_CLI.md)
- [`toranj-ncp` guide](README_NCP.md)

---

What does `"toranj"` mean? it's the name of a common symmetric weaving [pattern](https://en.wikipedia.org/wiki/Persian_carpet#/media/File:Toranj_-_special_circular_design_of_Iranian_carpets.JPG) used in Persian carpets.
