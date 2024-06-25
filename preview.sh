#!/bin/sh

Xephyr :5 -terminate -screen 1910x1030 &
sleep 5
DISPLAY=:5 ./moody
