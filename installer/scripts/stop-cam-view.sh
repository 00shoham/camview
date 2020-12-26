#!/bin/sh

killall -9 cam-monitor
killall -9 ffmpeg
ps -ef|grep grab-loop | grep -v grep | awk '{print $2}' | xargs kill -9

echo "Any cam-monitor processes left?"
ps -ef|grep cam-monitor|grep -v grep|grep -v libexec

echo "Any ffmpeg processes left?"
ps -ef|grep ffmpeg|grep -v grep

echo "Any grab-loop processes left?"
ps -ef|grep grab-loop|grep -v grep
