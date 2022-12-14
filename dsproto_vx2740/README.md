# Documentation for the DS-Proto VX2740

This package contains tools for Darkside's use of the CAEN VX2740 digitizers. It supports boards running both "Scope" and "Open" firmware.

## Developer notes

This repository uses the [git flow](https://www.atlassian.com/git/tutorials/comparing-workflows/gitflow-workflow) workflow.

* Commits should only be made to the `develop` branch, or a `feature/xyz` branch that branched from the `develop` branch.
* The `master` branch contains only tagged releases, that get merged from `develop` when things are stable. Please don't commit directly to `master`!
* `ReleaseNotes.md` gets updated whenever a new release is tagged on the master branch.

## Current status

The following executables are currently built:

* `vx2740_single_fe` will talk to a single VX2740 board, handling both configuration and data readout. Use the ODB to specify the hostname to talk to. Specify a "frontend index" using the `-i` flag to disambiguate settings for multiple boards (e.g. running with `-i 1`, you will configure the ODB at `/Equipment/VX2740_Config_001/Settings`).
* `vx2740_group_fe` allows talking to multiple VX2740 boards (multi-threaded), and optionally combining data into single midas events (based on the trigger #). You specify "default" parameters that apply to all boards, and can then apply board-specific "override" parameters if desired. This system will make it much nicer to configure the 100+ digitizers for DS-20k. You still need to specify a `-i` flag, this time to identify the group of digitizers to control (proto will only need 1 group of a few boards, but DS-20k may need 24 groups of 8-9 boards each). The number of boards to control is specified in the ODB. You will be told where to set the parameter after starting your frontend for the first time. Default parameters are set in `VX2740 defaults`, and board-level overrides in `/Equipment/VX2740_Config_Group_001/Settings/Board00` etc.
* `vx2740_test` is a trivial executable for testing connecting to a board. Specify a full "device path" like `Dig2:vx02` to connect to digitizer with name `vx02` using CAEN's `Dig2` library. It just connects then disconnects (or segfaults on Ubuntu 20.04 at TRIUMF...).
* `vx2740_dump_params` prints to screen all of the paramters that are available on the VX2740. Specify the hostname to connect to (e.g. `vx02`) and some options for filtering the output and adjusting the verbosity.
* `vx2740_dump_user_regs` prints to screen the values of user registers on the VX2740 (only sensible for User firmware, not the default Scope firmware). Specify the hostname to connect to and the start/end register range to dump (e.g. `vx02 0x100 0x1FC`). 
* `vx2740_poke` lets you get or set a single parameter on the board. It assumes you know the full path to the parameter (e.g. from running `vx2740_dump_params`). The result of the request is printed to screen. Useful for debugging the behaviour of certain board parameters. Example usage: `./vx2740_poke vx02 set /lvds/0/par/lvdsmode IORegister`.
* `dump_vx2740_data.py` will parse and print data to screen, either from a live experiment or a midas file.

## Known issues

* Sometimes the board gets stuck during the `reset()` command when starting the frontend. Ctrl+C the program, then start it again.

## Custom webpages

Two files (one HTML, one javascript) are provided to help you configure the digitizers through webpages. To use them, create two ODB keys as strings (you may need to create the `/Custom` ODB directory first):

* `/Custom/vx2740.js!` = `/path/to/dsproto_vx2740/custom/vx2740.js`
* `/Custom/vx2740 settings` = `/path/to/dsproto_vx2740/custom/vx2740.html`

After creating these two keys, you should see a new link on the left side of the midas status page titled `vx2740 settings`. You can then use that page to configure settings for all boards. There are two different modes, depending on whether you're using the "single" or "group" frontends. The correct mode is chosen based on whether you have ever run a "group" frontend.

Two more files are provided for using the save/load interface; see the "Saving/loading settings" section for more.

## Data format

The raw data read from the boards is in "network byte order", and requires lots of calls to `htonl()` to parse correctly. The frontend converts the data to "host byte order" (aka what users expect), so the data in midas banks can be read more easily.

There is a sample python program `dump_vx2740_data.py` that connects to a running experiment and will print a summary of each midas event (which may contain data from multiple boards if running the "group" frontend). See that code for an example of decoding the data in midas banks.

Note that in this repository we manipulate the "Open" firmware data so it is written in the same format as the "Scope" data. This makes parsing and comparing data from the two firmware versions easier, but is subject to change (if Darkside starts using some of the more advanced features of the Open firmware).

Special events are written at the start/end of each run. Normal events have variable length and start with 0x10, the "start run" event is 32 bytes long and begins with 0x30, and the "end run" event in 24 bytes long and begins with 0x32. See the VX2740 FELib manual for more details.

## Future plans

* Add support for Darkside-specific things (openFPGA registers, custom data format etc).

## Installation pre-requisites

You must install 2 libraries from the CAEN website - FELib and Dig2. Go to https://www.caen.it/download/?filter=VX2740, click on the "Software" tab, then scroll down to the "Middleware" section. Download the two GZ files for Linux.

CAEN_FELib dynamically loads CAEN_Dig2 at runtime - at no point does user code link against CAEN_Dig2!

### Setup

The instructions here assume you want to install the CAEN libraries locally (not globally in /lib etc). In this case, we install in `~/packages/caen_install`, and add that to `$LD_LIBRARY_PATH`.

    cd ~/packages
    mkdir caen_install
    export CAEN_INSTALL=~/packages/caen_install
    export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$CAEN_INSTALL/lib
    # FElib code uses a dlopen() to load the dig2 libraries, so we can’t just use -Wl,rpath=xxx when compiling to avoid using LD_LIBRARY_PATH
    # Also add to .bashrc

I assume that you already have midas installed and have set the `$MIDASSYS` environment variable.

### Notes for mac users

If you're running on a mac, you may need to make a couple of tweaks to the below instructions:

* After extracting the CAEN FELib library, but before compiling, edit `Makefile` and `src/Makefile` to remove `--no-as-needed` and `--as-needed` from the list of linker flags.
* After compiling the CAEN Dig2 library, make a symlink `ln -s $CAEN_INSTALL/lib/libCAEN_Dig2.dylib $CAEN_INSTALL/lib/libCAEN_Dig2.so` so that FELib can find the Dig2 library at runtime (it only looks for `.so` files).

### Install FELib

    cd ~/packages
    tar -xf caen_felib-v1.1.5.tar.gz
    cd caen_felib-v1.1.5
    ./configure --prefix=$CAEN_INSTALL/
    make
    make install

### Install boost (needed for Dig2 lib)

    cd ~/packages
    curl -L https://dl.bintray.com/boostorg/release/1.75.0/source/boost_1_75_0.tar.gz --output boost_1_75_0.tar.gz
    tar -xf boost_1_75_0.tar.gz
    export BOOST_PATH=$HOME/packages/boost_1_75_0

### Install Dig2, which needs access to boost and FELib headers

    cd ~/packages
    tar -xf caen_dig2-v1.4.3.tar.gz
    cd caen_dig2-v1.4.3
    CPATH=$CAEN_INSTALL/include:$BOOST_PATH ./configure --prefix=$CAEN_INSTALL
    CPATH=$CAEN_INSTALL/include:$BOOST_PATH make
    make install
    
### Install CAEN's demo code (optional)

    cd ~/packages/caen_dig2-v1.4.3/demo/caen-felib-demo-scope
    CPATH=$CAEN_INSTALL/include make

### Running CAEN's demo code (optional)

    cd ~/packages/caen_dig2-v1.4.3/demo/caen-felib-demo-scope
    ./caen-felib-demo-scope
    # When prompted for device path, enter: Dig2:<hostname> (e.g. Dig2:vx02)
    # After printing "starting acq_thread”, it’s waiting for user input (it’s not hung!). Use "t" to issue a software trigger, "s" to exit.
    
## Compiling this code

If you have the `$CAEN_INSTALL` environment variable defined, or you installed the CAEN libraries at the global level, you can just do:

    mkdir buiild
    cd build
    cmake ..
    make install
    
(On some systems, you may need to use `cmake3` instead of `cmake`).

If you want to manually specify where the CAEN libraries and headers are, you can pass more options to `cmake`:

    cmake -DCAEN_FELIB_LIBDIR=/path/to/libdir -DCAEN_FELIB_INCDIR=/path/to/include
    
