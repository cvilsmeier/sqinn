#!/bin/sh

set -e
stat lib/ > /dev/null  # must be in correct dir
rm -rf bin/
