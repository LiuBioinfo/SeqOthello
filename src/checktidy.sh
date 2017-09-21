#!/bin/bash
clang-tidy $1 -- -I../include/ -I../lib/ --std=gnu++11 -header-filter=.*

