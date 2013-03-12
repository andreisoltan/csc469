#!/usr/bin/perl

use strict;

my $dir=$ARGV[0];
my $name;

#foreach $name ("cache-scratch", "cache-thrash", "threadtest", "larson") {
foreach $name ("cache-scratch") {
  print "name = $name\n";
  print "running $dir/$name/runours.pl";
  system "$dir/$name/runours.pl $dir/$name";
  
}


