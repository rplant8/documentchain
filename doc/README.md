DMS Core
=====================

This is the official reference wallet for DMS digital currency and comprises the backbone of the Documentchain peer-to-peer network. You can [download DMS Core](https://github.com/Krekeler/documentchain/releases) or [build it yourself](#building) using the guides below.

Running
---------------------
The following are some helpful notes on how to run DMS on your native platform.

### Unix

Unpack the files into a directory and run:

- `bin/dms-qt` (GUI) or
- `bin/dmsd` (headless)

### Windows

Run "dmscore-...-setup.exe" or unpack the ZIP file into a directory, and then run dms-qt.exe.

### macOS (formerly OS X)

Open the .dmg file and drag DMS-Qt to your applications folder, and then run DMS-Qt.

### Need Help?

* See the [Supportpages](https://documentchain.org/support/) for help and more information
* DMS is forked from Dash, see [Dash documentation](https://docs.dash.org/en/stable/)

Building
---------------------
The following are developer notes on how to build DMS Core on your native platform. They are not complete guides, but include notes on the necessary libraries, compile flags, etc.

- [macOS Build Notes](build-osx.md)
- [Unix Build Notes](build-unix.md)
- [Windows Build Notes](build-windows.md)
- [OpenBSD Build Notes](build-openbsd.md)
- [Gitian Building Guide](gitian-building.md)

Development
---------------------
The DMS Core repo's [root README](/README.md) contains relevant information on the development process and automated testing.

- [Developer Notes](developer-notes.md)
- Release Notes
- [Release Process](release-process.md)
- [Translation Process](translation_process.md)
- [Translation Strings Policy](translation_strings_policy.md)
- [Travis CI](travis-ci.md)
- [Unauthenticated REST Interface](REST-interface.md)
- [Shared Libraries](shared-libraries.md)
- [BIPS](bips.md)
- [Dnsseed Policy](dnsseed-policy.md)
- [Benchmarking](benchmarking.md)

### Miscellaneous
- [Assets Attribution](assets-attribution.md)
- [Files](files.md)
- [Reduce Traffic](reduce-traffic.md)
- [Tor Support](tor.md)
- [Init Scripts (systemd/upstart/openrc)](init.md)
- [ZMQ](zmq.md)

License
---------------------
Distributed under the [MIT software license](/COPYING).
This product includes software developed by the OpenSSL Project for use in the [OpenSSL Toolkit](https://www.openssl.org/). This product includes
cryptographic software written by Eric Young ([eay@cryptsoft.com](mailto:eay@cryptsoft.com)), and UPnP software written by Thomas Bernard.
