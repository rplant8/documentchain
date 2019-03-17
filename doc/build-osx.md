macOS (OS X) Build Instructions and Notes
====================================
The commands in this guide should be executed in a Terminal application.
The built-in one is located in `/Applications/Utilities/Terminal.app`.

Preparation
-----------
1. Install the macOS command line tools:

    `xcode-select --install`

	When the popup appears, click `Install`.

2. Install [Homebrew](https://brew.sh).

	*If you get "Permission denied": `sudo chown -R $USER:admin /usr/local`  [can help](https://github.com/Homebrew/legacy-homebrew/issues/19670).*

Dependencies
----------------------

    brew install automake berkeley-db4 libtool boost --c++11 miniupnpc openssl pkg-config protobuf qt libevent librsvg

Build DMS Core
------------------------

1. Download the source

        git clone https://github.com/Krekeler/documentchain

2. Build DMS Core

	Configure and build the headless binaries dmsd, dms-cli and dms-tx.

        cd documentchain
        ./autogen.sh
        ./configure
        make

3. GUI Wallet

	Build the DMS-Qt.app and DMS-Qt.dmg that contains the .app bundle:

        make deploy

Running
-------

### GUI Wallet dms-qt

Run "DMS Core" (DMS-Qt.app)

### Daemon dmsd

DMS Core is now available at `./src/dmsd`

Before running, it's recommended you create an RPC configuration file.

    echo -e "rpcuser=dmsrpc\nrpcpassword=$(xxd -l 16 -p /dev/urandom)" > "/Users/${USER}/Library/Application Support/DMSCore/dms.conf"

    chmod 600 "/Users/${USER}/Library/Application Support/DMSCore/dms.conf"

The first time you run the wallet, it will start downloading the blockchain. This process could take several hours.

You can monitor the download process by looking at the debug.log file:

    tail -f $HOME/Library/Application\ Support/DMSCore/debug.log

### Other commands:

    ./src/dmsd -daemon # Starts the dms daemon.
    ./src/dms-cli --help # Outputs a list of command-line options.
    ./src/dms-cli help # Outputs a list of RPC commands when the daemon is running.

Using Qt as IDE
------------------------
Download and install the open source edition of [Qt](https://www.qt.io/download/).
Check newest Qt version during installation process.

1. Make sure you installed everything through Homebrew mentioned above
2. Do a proper `./autogen.sh` and `./configure --with-gui=qt5 --enable-debug`
3. In Qt Creator do "New Project" -> Import Project -> Import Existing Project
4. Enter "dms-qt" as project name, enter src/qt as location
5. Leave the file selection as it is
6. Confirm the "summary page"
7. In the "Projects" tab select "Manage Kits..."
8. On page "Kits" select the default "Desktop" kit and set the compiler to "Clang (x86 64bit in /usr/bin)"
9. Select LLDB as debugger (you might need to set the path to your installation)
10. In the "Projects" tab select "Build Settings", expand "Build Setps" and uncheck the make target "all"
11. Start building and debugging with Qt Creator
