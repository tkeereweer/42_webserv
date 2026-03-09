#!/bin/bash

siege -v -c 50 -t 1M http://server:9090/cgi-bin/getform_handle.py?firstname=Form&lastname=StressTest&sex=male&email=stressing.out%40justtakeiteasy.ch&about_you=this+is+the+form+for+the+stress+test%21&annoying_status=true


