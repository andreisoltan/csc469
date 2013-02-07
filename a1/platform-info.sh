#!/bin/bash

# Spit out some info about this machine

uname -a

echo "### cpuinfo"
sed -n -e '/^cpu MHz/p' -e '/^processor/p' -e '/^model name/p' < /proc/cpuinfo

echo "### meminfo"
sed -n '/MemTotal/p' < /proc/meminfo

echo "### sched_mc_power_savings"
cat /sys/devices/system/cpu/sched_mc_power_savings 2>&1

echo "### cpu0/scaling_governor"
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>&1

echo "### cpu0/cpufreq/scaling_available_frequencies"
cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_available_frequencies 2>&1
