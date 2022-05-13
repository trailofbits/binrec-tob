BinRec: Dynamic Binary Lifting and Recompilation
================================================

Overview
--------

BinRec dynamically lifts binary executables to LLVM IR, where they can be optimized, hardened, or transformed and recompiled back to binaries. It is based on
[S2E](http://s2e.systems/docs/). For a full description of the framework, see the original research [paper](https://dl.acm.org/doi/10.1145/3342195.3387550).


Key Dependencies and Environment
--------------------------------

Binrec has been developed for and tested against the following environments and major dependencies:

- Ubuntu: 20.04.03
- LLVM (Clang): 12
- Python 3.9
- s2e-env @778116b

The limiting factor for both Linux environment and LLVM is s2e, which supports Ubuntu 20.04 LTS at a maximum.

Installing BinRec
---------------
1. BinRec uses [just](https://github.com/casey/just#installation) to automate various tasks including building BinRec. The first step in building BinRec is to install this tool (and `curl` if not already installed). We provide a simple shell script for this:

        $ ./get_just.sh

2. Next, BinRec and its dependencies can be fetched, built, and installed from the root of this repository with:

       $ just install-binrec

3. (Optional) The above command will download a pre-built QEMU virtual machine image within which target binaries will run. The default pre-built image is a Linux `x86` image (currently the only supported environment). If you want to download additional images (e.g., Linux `x64`) you can use the following command:

   ```bash
   # just build-s2e-image <image_name>
   $ just build-s2e-image debian-9.2.1-x86_64
   ```


Using BinRec 
-------------
BinRec is a versatile dynamic binary recorvery and recompilation platform. As such, it can be used to achieve many different goals at varying levels of complexity. Please refer to BinRec's [User Manual](docs/manual/manual.md) for detailed walkthroughs of typical workflows.


Other Notes
-----------

### Callbacks

Support for callbacks is currently work in progress. To enable it, set the lift and lower option `-f unfallback`. Note
this does not actually enable the fallback, it will use whatever fallback option you have hardcoded. Later the option
can be made more clear.

--------
