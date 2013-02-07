#!/usr/bin/env python

from period_plot import PeriodPlot

import os
import re
import subprocess
import sys

def cmd(command):
  '''Runs the given shell command and returns its standard output as a string.
     Throws an exception if the command returns anything other than 0.'''
  p = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  ret = os.waitpid(p.pid, 0)[1]
  if ret != 0:
    raise Exception('Command [%s] failed with status %d\n:%s' % (command, ret, p.stderr.read()))
  return p.stdout.read()


# Parse the number of periods from the command line arguments.
num = int(sys.argv[1]);

# Build the program.
cmd('make')

# Run the program and create the plot.
output = cmd(['./random_periods', str(num)])
plot = PeriodPlot()
for line in output.splitlines():
  # Parse the program's output.
  match = re.match('(start=)(.*)(,)(end=)(.*)', line) 
  start = float(match.groups()[1])
  end = float(match.groups()[4])
  plot.AddPeriod(start, end, "red");
plot.CreatePlot('Random Periods', 'random.eps')
