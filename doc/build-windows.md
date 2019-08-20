WINDOWS BUILD NOTES
====================

Below are some notes on how to build DMS Core for Windows.

Most developers use cross-compilation from Ubuntu to build executables for
Windows. This is also used to build the release binaries.

While there are potentially a number of ways to build on Windows (for example using msys / mingw-w64),
using the Windows Subsystem For Linux is the most straightforward. If you are building with
another method, please contribute the instructions here for others who are running versions
of Windows that are not compatible with the Windows Subsystem for Linux.

Compiling with Windows Subsystem for Linux (WSL)
-------------------------------------------

With Windows 10, Microsoft has released a new feature named the [Windows
Subsystem for Linux](https://msdn.microsoft.com/commandline/wsl/about). This
feature allows you to run a bash shell directly on Windows in an Ubuntu-based
environment. Within this environment you can cross compile for Windows without
the need for a separate Linux VM or server.

This feature is not supported in versions of Windows prior to Windows 10 or on
Windows Server SKUs. In addition, it is available only for 64-bit versions of
Windows.

To get the bash shell, you must first activate the feature in Windows.

1. Follow [Windows Subsystem for Linux Installation](https://docs.microsoft.com/en-us/windows/wsl/install-win10)
   and install Debian from Microsoft Store.
   You can also use Ubuntu 18.04 LTS.
1. Create a new UNIX user account (this is a separate account from your Windows account).
1. It is recommended to restart the host computer (Windows PC).

After the bash shell is active, you can follow the instructions below.

Cross-compilation
-------------------

These steps can be performed on WSL or, for example, an VM. The depends system
will also work on other Linux distributions, however the commands for
installing the toolchain will be different.

In WSL the entire windows path is part of the linux $PATH variable and it interferes with the
make command. We have to clean $PATH or can add "PATH=$(getconf PATH)" to the make command.
See https://github.com/bitcoin/bitcoin/pull/10889

First, install the general dependencies:

    sudo apt-get update
    sudo apt-get upgrade
    sudo apt-get install build-essential libtool autotools-dev automake pkg-config bsdmainutils curl git

A host toolchain (`build-essential`) is necessary because some dependency
packages (such as `protobuf`) need to build host utilities that are used in the
build process.

### BerkeleyDB is required for the wallet

For Ubuntu only: db4.8 packages are available [here](https://launchpad.net/~bitcoin/+archive/bitcoin).
You can add the repository and install using the following commands:

    sudo apt-get install software-properties-common
    sudo add-apt-repository ppa:bitcoin/bitcoin
    sudo apt-get install libdb4.8-dev libdb4.8++-dev

Ubuntu and Debian have their own libdb-dev and libdb++-dev packages, but these will install
BerkeleyDB 5.1 or later. This will break binary wallet compatibility with the distributed executables, which
are based on BerkeleyDB 4.8. If you do not care about wallet compatibility,
pass `--with-incompatible-bdb` to configure.

### Installer

If you want to build the windows installer with `make deploy` you need NSIS:

    sudo apt-get install nsis

### Download the source

    git clone https://github.com/Krekeler/documentchain.git

### Building for 64-bit Windows

To build executables for Windows 64-bit, install the following dependencies:

    sudo apt-get install g++-mingw-w64-x86-64

Set the default mingw32 g++ compiler option to posix:

    sudo update-alternatives --config x86_64-w64-mingw32-g++
	
Then build using:

    cd documentchain/depends
    make HOST=x86_64-w64-mingw32 PATH=$(getconf PATH)
    # if you get a "Missing DLL" error, try it without the PATH statement
    cd ..
    ./autogen.sh # not required when building from tarball
    CONFIG_SITE=$PWD/depends/x86_64-w64-mingw32/share/config.site ./configure --prefix=/
    make

### Building for 32-bit Windows

To build executables for Windows 32-bit, install the following dependencies:

    sudo apt install g++-mingw-w64-i686 mingw-w64-i686-dev
	
Set the default mingw32 g++ compiler option to posix:

    sudo update-alternatives --config i686-w64-mingw32-g++

Then build using:

    cd depends
    make HOST=i686-w64-mingw32 PATH=$(getconf PATH)
    # if you get a "Missing DLL" error, try it without the PATH statement
    cd ..
    ./autogen.sh # not required when building from tarball
    CONFIG_SITE=$PWD/depends/i686-w64-mingw32/share/config.site ./configure --prefix=/
    make

## Depends system

For further documentation on the depends system see [README.md](../depends/README.md) in the depends directory.

Installation
-------------

After building using the Windows subsystem it can be useful to copy the compiled
executables to a directory on the windows drive in the same directory structure
as they appear in the release `.zip` archive. This can be done in the following
way. This will install to `c:\workspace\documentchain`, for example:

    make install DESTDIR=/mnt/c/workspace/documentchain

Using Qt to review source and forms
------------------------
Download and install the open source edition of [Qt](https://www.qt.io/download/).
Check newest Qt version during installation process.

1. In Qt Creator do "New Project" -> Import Project -> Import Existing Project
1. Enter "dms-qt" as project name, enter src/qt as location
1. In file selection add src/forms
1. Confirm the "summary page"
