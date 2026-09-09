#!/usr/bin/env zsh
set -eu
mkdir -p build
ffmpeg -i assets/example.m4a \
    -filter_complex "color=c=black:s=1280x720[bg];[0:a]showwaves=s=1280x720:mode=cline:colors=00FF00[wave];[bg][wave]overlay[v]" \
    -map "[v]" \
    -map 0:a \
    -c:v libx264 \
    -c:a aac \
    -shortest build/example.mp4 
