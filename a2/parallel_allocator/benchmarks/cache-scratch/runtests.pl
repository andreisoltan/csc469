#!/usr/bin/perl

use strict;

my $name;

my $dir = $ARGV[0];

#Ensure existence of $dir/Results
if (!-e "$dir/Results") {
    mkdir "$dir/Results", 0755
	or die "Cannot make $dir/Results: $!";
}

foreach $name ("libc", "kheap", "cmu") {
    print "name = $name\n";
    # Create subdirectory for current allocator results
    if (!-e "$dir/Results/$name") {
	mkdir "$dir/Results/$name", 0755
	    or die "Cannot make $dir/Results/$name: $!";
    }

    # Run tests for 1 to 8 threads
    for (my $i = 1; $i <= 8; $i++) {
	print "Thread $i\n";
	my $cmd1 = "echo \"\" > $dir/Results/$name/cache-scratch-$i";
	system "$cmd1";
	for (my $j = 1; $j <= 5; $j++) {
	    print "Iteration $j\n";
	    my $cmd = "$dir/cache-scratch-$name $i 1000 8 100000 >> $dir/Results/$name/cache-scratch-$i";
	    print "$cmd\n";
	    system "$cmd";
	}
    }
}


