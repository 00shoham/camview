#!/bin/sh

(find . -type f -name '*.c';\
 find . -type f -name '*.h';\
 find . -type f -name '*.sh';\
 find . -type f -name '*.tex';\
 find . -type f -name '*.txt';\
 find . -type f -name '*.css';\
 find . -type f -name '*.js';\
 find . -type f -name '*.template';\
 find . -type f -name 'Makefile';\
 find config -type f;\
 ) | sort | uniq | xargs wc -l

