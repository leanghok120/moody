#!/bin/sh

Xephyr :5 -terminate -screen 1910x1030 &
sleep 3
DISPLAY=:5 ./moody
