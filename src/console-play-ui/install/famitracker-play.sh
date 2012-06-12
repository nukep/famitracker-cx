#!/bin/bash

ROOT=$(cd "${0%/*}" && echo $PWD)

cd "$ROOT"

export LD_LIBRARY_PATH=./lib:$LD_LIBRARY_PATH

exec ./bin/famitracker-play $@

