#!/bin/bash

# Counts the number of (unique) users logged on to this system
who -q | head -n1 | sed -e 's/ /\n/g' -e '/^$/d' | sort | uniq | wc -l
