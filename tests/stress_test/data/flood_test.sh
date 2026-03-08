#!/bin/bash

siege -b -c 255 http://server:9090/index.html

#warning: consumes 100% CPU !!