#!/usr/bin/perl

use strict;

my $dir=$ARGV[0];
my $name;

#foreach $name ("cache-scratch", "cache-thrash", "threadtest", "larson") {
foreach $name ("cache-thrash") {
  print "name = $name\n";
  print "running $dir/$name/runtests.pl";
  system "$dir/$name/runtests.pl $dir/$name";
  
}


