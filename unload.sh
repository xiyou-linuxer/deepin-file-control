#!/bin/bash
PATHS=`pwd`
REA+=$PATHS
REA+="/test/myhook.so"    
rm -rf $REA
`echo "" > /etc/ld.so.preload` 
