#!/bin/bash

`gcc -fPIC -shared -o tests/hook.so src/hook.c -ldl`
`g++ -o tests/cli src/cli_main.cpp -lpthread -ldl`
`g++ -o tests/ser src/ser_main.cpp -lpthread -ldl`
