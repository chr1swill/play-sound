#!/bin/sh

BIN=bin
CFLAGS="-Wall -Wextra -ggdb"

set -xe

if [ -d "$BIN" ]; then
  rm -rf "$BIN"
fi

mkdir -p "$BIN"

gcc ${CFLAGS} -o ${BIN}/main main.c arena.c -lsqlite3
