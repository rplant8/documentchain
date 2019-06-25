#!/bin/bash
# use testnet settings,  if you need mainnet,  use ~/.dmscore/dmsd.pid file instead
dms_pid=$(<~/.dmscore/testnet3/dmsd.pid)
sudo gdb -batch -ex "source debug.gdb" dmsd ${dms_pid}
