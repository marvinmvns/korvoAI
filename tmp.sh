#!/bin/bash
/home/bigfriend/.platformio/penv/bin/pio device monitor --environment esp32-korvo-v1_1 > serial_log.txt 2>&1 &
MONITOR_PID=$!
sleep 15
kill $MONITOR_PID
