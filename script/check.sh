#!/bin/sh

set -e
stat lib/ > /dev/null  # must be in correct dir
go run script/check.go # run checks, needs go
