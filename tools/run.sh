#!/usr/bin/env zsh
set -eu
mkdir -p build
clear
sleep 2
gcc -std=gnu2x -Wall -Wextra -o ./build/story-teller src/story-teller.c
./build/story-teller ./assets/story.txt 21 50
sleep 2 
