#!/usr/bin/perl

use strict;

my $dir = $ARGV[0]; shift;
my $name = shift || "amalloc-dbg";
my $threads = shift || 8;
my $iterations = shift || 5;

#Ensure existence of $dir/Results
if (!-e "$dir/Results") {
    mkdir "$dir/Results", 0755
	or die "Cannot make $dir/Results: $!";
}

print "name = $name\n";
# Create subdirectory for current allocator results
if (!-e "$dir/Results/$name") {
mkdir "$dir/Results/$name", 0755
    or die "Cannot make $dir/Results/$name: $!";
}

# Run tests for 1 to 8 threads
for (my $i = 1; $i <= $threads; $i++) {
#    for (my $i = 1; $i <= 2; $i++) {
print "Thread $i\n";
my $cmd1 = "echo \"\" > $dir/Results/$name/cache-thrash-$i";
system "$cmd1";
for (my $j = 1; $j <= $iterations; $j++) {
    print "Iteration $j\n";
    my $cmd = "$dir/cache-thrash-$name $i 1000 8 100000 >> $dir/Results/$name/cache-thrash-$i";
    print "$cmd\n";
    system "$cmd";
}
}
