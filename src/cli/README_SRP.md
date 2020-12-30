# OpenThread CLI - SRP (Service Registration Protocol)

## Quick Start

### Build with SRP Client support

Use the `SRP_CLIENT=1 ECDSA=1` build switches to enable SRP Client API support (which requires ECDSA support).

```bash
> ./bootstrap
> make -f examples/Makefile-simulation SRP_CLIENT=1 ECDSA=1
```

## Example

Print supported modes, `client`.

```bash
> srp
client
Done
```

## CLI Reference

- [SRP Client CLI Reference](README_SRP_CLIENT.md)
