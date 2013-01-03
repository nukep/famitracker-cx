#!/bin/bash

ROOT=$(cd "${0%/*}" && echo $PWD)

export LD_LIBRARY_PATH="$ROOT"/lib:$LD_LIBRARY_PATH
exec "$ROOT"/bin/famitracker-qt "$@"

