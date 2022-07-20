# OpenThread Co-processor Remote Procedure Call Module Reference

The OpenThread Co-processor Remote Procedure Call module is a tool for debugging platform hardware manually.

It is very similar to the Factory Diags module, but does not require having commands as subcommands of "diag". This allows for user functions to be reused between SoC images and RCP images without and changes to the commands a user inputs to the CLI.

For example, say a user has a `get_temp` function which gets the value of some temperature sensor on a RCP or SoC device.

With the "diag" module, the user would use the following CLI commands:

- **Host/RCP**: "diag get_temp sensor2"
- **SoC**: "get_temp sensor2"

With the Co-processor Remote Procedure Call module, the CLI commands would be the same for both environments.

- **Host/RCP**: "get_temp sensor2"
- **SoC**: "get_temp sensor2"
