A contrived example of automating an experiment with Gnuplot.

To use these files, simply run
  ./driver.py num
where num is the number of periods you want to generate. This script will create
an image random.eps displaying the num randomly sized intervals.


files:
  driver.py - runs the experimental program (random_periods), parses its output,
              and creates the image.
  Makefile - compiles random_periods.c
  period_plot.py   - python class for creating period plots with Gnuplot.
  random_periods.c - sample data-generating program

requirements:
  python 2.6
  gnuplot 4.2 patchlevel 2
  gcc
