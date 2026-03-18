#!/bin/bash

siege -v -b -c 200 -t 1M http://server:9090/index.html
