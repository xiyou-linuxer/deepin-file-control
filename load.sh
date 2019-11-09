#!/bin/bash
PATHS=`pwd`
REA+=$PATHS
REA+="/test/myhook.so"                                                                
`echo $REA > /etc/ld.so.preload`
