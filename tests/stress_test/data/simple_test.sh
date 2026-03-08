#!/bin/bash

siege -v -c 50 -t 1M http://server:9090/index.html
