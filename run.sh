#!/bin/bash

set -xeuo pipefail

LD_LIBRARY_PATH=./raylib-5.5_linux_amd64/lib ./main $@
