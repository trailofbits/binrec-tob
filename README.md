BinRec: Off-the-shelf Program Recompilation Through Binary Recovery
===================================================================

Change Notes
--------

Trace merging scripts deduplicated. Assumes you pass the s2e-max-processes flag for directory
structure. See the Deinstrumenting and lowering bitcode section 

Support for callbacks is currently work in progress. To enable it, set the lift and lower 
option -f unfallback. Note this does not actually enable the fallback, it will use 
whatever fallback option you have hardcoded. Later the option can be made more clear

Overview
--------

BinRec dynamically lifts binary executables to LLVM IR. It is based on
[S2E][1]. For a full description of the framework, see our [paper][8].


Building BinRec using Docker
----------------------------

Since BinRec is based on S2E, it has the same build dependencies (plus some
additional packages for runscripts and compiling tests). The version of S2E
used has a hard requirement on Ubuntu 14.04, so the build system may break on
different distributions. To make things easier, we use [Docker][5] to confine
the build environment to an isolated container without polluting the host
system. The Docker instance runs as a service, started once using [Docker
Compose][6], to be used with the `docker exec` command for which we have a nice
script in [docker/run](/docker/run). The [compose
file](/docker/docker-compose.yml) mounts the root directory and synchronizes
file permissions between the host and the Docker instance.

Here follows a short step-by-step guide to building BinRec:

1.  Clone this repository.

        $ git clone https://github.com/securesystemslab/BinRec.git 
        $ git checkout branch/you/want

1.  Set environment variables

        $ cd binrec
        $ . scripts/env.sh
        
1. (Multicompiler) Fetch multicompiler source. This is done outside docker 
	to avoid authentication issues
	
		$ cd docker
		$ git clone https://github.com/Sisyph/multicompiler-automation.git
		$ cd multicompiler-automation
		$ make fetch
1.  Invoke script wrapping docker-compose up (this may take a while).

        $ ../start
        
1. (Multicompiler) Build the multicompiler and move its artifacts to a docker volume.

		$ ../runBuild bash
			this will give you a shell inside the container to build the multicompiler
		$ cd /src
		$ make install (may prompt for sudo to patch libc printf)
		$ exit

1.  Build BinRec using the Docker service. This will definitely take a while,
    so go get a coffee as the S2E documentation suggests...

        $ cd $S2EDIR
        $ ./docker/run bash
        $ make build && make

    This also builds the LLVM passes needed to post-process the output of the
    S2E plugins.
    
1.  You can use the network to put stuff into your qemu vm. Lifting binaries that 
    actually use the network is untested. Run the following script. Read the contents and do that if need be

        $ ./scripts/netconfig.sh (enter sudo password)

You can use Docker Compose again later to stop and clean up the service when
you are done with building and running BinRec:

    $ ./docker/stop


Building BinRec without Docker
------------------------------

We currently have no documentation for this, other than the setup of the files
in the [docker](/docker/) directory. You may also want to look at the [guide
for building S2E][2].


Running a binary in the BinRec front-end
----------------------------------------

### Preparing a VM image

You need a QEMU VM image to run the binary in the front-end. Follow [S2E's
guide for building a VM image][3] to create a Debian image, or use our
pre-built image from [here][7]. Save the image to `qemu/debian.raw`.

**WARNING:** The downloaded image name is `binrec-debian.raw`. Rename it to `debian.raw`.

We use S2E's [init\_env.so][4] library to detect execution of the target binary
in the VM. Our pre-built image has a built copy integrated, but for a custom
image you need to build the library (along with the `s2eget` tool) in
[s2e/guest/tools](/s2e/guest/tools/) and copy them into the VM.

### Running the binary

S2E builds two QEMU binaries: one unmodified and one with S2E instrumentation
to facilitate symbolic execution and plugins. The latter is considerably slower
and does not support KVM mode, so you want to do as much set-up work (like
booting the VM) in the unmodified version as possible. Another thing to take
into consideration is that the target binary needs to be in the VM, which does
not support mounting host directories, so you need to either copy the files
into the VM on beforehand or load them at runtime. Here, we give an example of
the `hello` program from the `test` directory being recovered using our
pre-built VM image in combination with S2E's "HostFiles" plugin.

1.  Boot the VM:

        $ ./docker/run bash         # open a shell in the docker instance
        $ export MEM_AMOUNT=1024      # if you want to modify VM ram, default is 1G 
        $ ./qemu/debian.sh -vnc :0  # boot the VM

    Pass -net if you want the qemu to access the network. Put it before vnc arg 

        $ ./qemu/debian.sh -net -vnc :0   # boot the VM

2.  In a different shell, use a VNC client to connect to the VM at the port 
    reported by your environment configuration step:

        $ xvncviewer localhost::$VNCPORT

    Log in (user "dev" and password "power" for the prebuilt image) and run the
    command `getrun-cmd`:

        $ getrun-cmd

    This command is a wrapper that adds input arguments to `s2eget`, which
    blocks in regular QEMU and continues in S2E's modified version of QEMU, so
    we can now create a snapshot to continue later in S2E mode.  Switch to the
    QEMU shell by pressing ctrl+alt+2 and save a snapshot with the name "cmd".
    Note, the directory from which you invoke getrun-cmd is important is some cases.
    Then, exit the VM:
 
        # savevm cmd
        # quit
    
    **WARNING:** In macOS, if `ctrl+alt+2` doesn't work, try the following:
    `XQUARTS-->preferences-->Input-->Option keys send ALT L and ALT R`
    
3.  Back in the docker instance shell, load the snapshot in S2E mode with the
    custom plugin loaded, using the HostFiles plugin to copy the target binary
    to the VM:

        $ ./qemu/cmd-debian.sh --vnc 0 hello

    This loads the "cmd" snapshot and copies the "eq" binary into the VM as
    requested by the `getrun-cmd` command. `--vnc` makes the script pass `-vnc
    :0` to the qemu command, making it start a VNC server since we are running
    in a docker instance where there is no X server. Run `./qemu/cmd-debian.sh
    -h` to see options on how to pass input to the target binary. Note that the
    target binary should reside in directories scanned by the HostFiles plugin,
    configured in `plugins/config.debian.lua`.
    Also see https://github.com/S2E/s2e-old/blob/master/docs/Howtos/init_env.rst

4. Parallel lifting is supported by creating a config file which contains the binary invocation and args,
    then using the cmd-debian-mt script. Edit this script, for instance to pass --net to the cmd-debian invocations

        $ ./qemu/cmd-debian-mt.sh configFile

5. Partial SPEC2006 instructions
        
        $ Install spec2006 at $S2EDIR/test/spec2006. In $S2EDIR/plugins/config.debian.lua uncomment the paths
        to find the binaries 
        $ use the network to copy all the desired spec inputs into your qemu vm. make sure to invoke getrun-cmd 
        where these input files are on the path
        $ use the specConf config file for cmd-debian-mt

    The VM is killed automatically after the target binary has been run. Directories
    called `s2e-out-eq-1` `s2e-out-eq-2` are created, containing the captured 
    LLVM bitcode in `captured.bc`.


Deinstrumentation and lowering of captured bitcode
--------------------------------------------------

Before we run the lifting script, we need to merge these multiple directories into 1 
directory called `s2e-out-eq`. To do that run following in `$S2EDIR` if you want to merge all 
directories \**-eq-*\*

    $ ./scripts/double_merge.sh eq

or run the following if you want to merge the numbered subdirectories of s2e-out-eq-1

    $ ./scripts/merge_traces.sh eq-1

Now go into the `s2e-out-eq` directory and run the lifting script to obtain deinstrumented
code.

    $ cd s2e-out-eq
    $ ../scripts/lift.sh captured.bc lifted.bc
    $ # Here you can transform lifted.bc using LLVM's opt command
    $ ../scripts/lower.sh lifted.bc recovered.bc

This created a recovered binary called `recovered` that contains the compiled
code from `recovered.bc` (LLVM source code is generated in `recovered.ll`).

Recovered code will be exposed to the dynamic linker by default. To instead expose
the original code pass the flags "-n 1" to the lift and lower scripts.
WARNING  use this flag for c++ programs, since there is a bug with recoverd lifting

There are a number of other scripts you can check out as well (the names and
contents are fairly self-explanatory). For example, to optimized the lifted
code and disable the fallback mechanism, run:

    $ ../scripts/optimize.sh -f none captured.bc recovered-optimized.bc

Fallback Testing
--------------------------------------------------

Follow the above procedure but use the program bzip2 instead of hello. The
executable produced with lift & lower scripts should include the extended 
fallback and jump to the old binary. An executable produced with the optimize
script will get a segfault when the fallback is invoked. 

[1]: https://github.com/dslab-epfl/s2e/blob/master/docs/index.rst
    "S²E documentation"
[2]: https://github.com/dslab-epfl/s2e/blob/master/docs/BuildingS2E.rst
    "Building S²E"
[3]: https://github.com/dslab-epfl/s2e/blob/master/docs/ImageInstallation.rst
    "Preparing VM Images for S²E"
[4]: https://github.com/dslab-epfl/s2e/blob/master/docs/Howtos/init_env.rst
    "init_env.so preload library"
[8]: https://github.com/dslab-epfl/s2e/blob/master/docs/UsingS2EGet.rst
    "Quickly Uploading Programs to the Guest with s2eget"
[5]: https://www.docker.com/ "Docker"
[6]: https://www.docker.com/products/docker-compose "Docker Compose"
[7]: https://drive.google.com/open?id=12Oe1DlPH6ZoJMDgxW08eWqoYXfNVaatS
    "BinRec Debian image"
[8]: https:/tkroes.nl/papers/binrec-asplos-2017.pdf
    "BinRec: Off-the-shelf Program Recompilation Through Binary Recovery"
