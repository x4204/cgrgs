#!/bin/bash

set -xeuo pipefail

gcc \
  -Wall \
  -Wextra \
  -I./raylib-5.5_linux_amd64/include \
  -o main \
  main.c \
  -L./raylib-5.5_linux_amd64/lib \
  -lraylib \
  -lm
