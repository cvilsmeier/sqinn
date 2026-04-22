#!/bin/sh

set -e
stat lib/ > /dev/null  # must be in correct dir
which go  > /dev/null  # must have go command installed
go run script/check.go # run checks, needs go
