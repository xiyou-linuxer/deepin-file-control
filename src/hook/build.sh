#!/bin/bash
`gcc -fPIC -shared -o ../../test/myhook.so myhook.c -ldl`
