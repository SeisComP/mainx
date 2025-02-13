# SeisComP mainx repository

This repository contains SeisComP applications which extend the
default applications from the main repository or add new
prototypes and experiments.

The repository cannot be built standalone. It needs to be integrated
into the `seiscomp` build environment and checked out into
`src/base/mainx`.

```
$ git clone [host]/seiscomp.git
$ cd seiscomp/src/base
$ git clone [host]/mainx.git
```

This repository is compatible with SeisComP Nightly or 7.x and above. Do not
try to compile it with SeisComP 6.x or ealier.

# Build

## Configuration

There are no special configuration options right now.

## Compilation

Follow the build instructions from the `seiscomp` repository.

