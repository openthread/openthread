# Security Considerations #

## Raw Application Access ##

Spinel **MAY** be used as an API boundary for allowing processes to configure
the NCP. However, such a system **MUST NOT** give unprivileged processess the
ability to send or receive arbitrary command frames to the NCP. Only the
specific commands and properties that are required should be allowed to be
passed, and then only after being checked for proper format.
