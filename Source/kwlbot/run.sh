#!/bin/bash

LD_LIBRARY_PATH_=$LD_LIBRARY_PATH

export LD_LIBRARY_PATH=.:$LD_LIBRARY_PATH

./kwlbot $*

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH_
