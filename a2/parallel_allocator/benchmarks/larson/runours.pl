#!/usr/bin/perl

use strict;

my $name;

my $dir = $ARGV[0];

#Ensure existence of $dir/Results
if (!-e "$dir/Results") {
    mkdir "$dir/Results", 0755 
	or die "Cannot make $dir/Results: $!";
}

foreach $name ("amalloc-dbg") {
    print "name = $name\n";
    # Create subdirectory for current allocator results
    if (!-e "$dir/Results/$name") {
	mkdir "$dir/Results/$name", 0755 
	    or die "Cannot make $dir/Results/$name: $!";
    }
    # Run tests for 1 to 8 threads
    for (my $i = 1; $i <= 8; $i++) {
	print "Thread $i\n";
	my $cmd1 = "echo \"\" > $dir/Results/$name/larson-$i";
	system "$cmd1";
	for (my $j = 1; $j <= 5; $j++) {
	    print "Iteration $j\n";
	    my $cmd = "$dir/larson-$name < $dir/Input/small-$i-threads >> $dir/Results/$name/larson-$i 2>&1";
	    print "$cmd\n";
	    system "$cmd";
	}
    }
}


