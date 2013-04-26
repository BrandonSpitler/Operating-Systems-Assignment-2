use Time::HiRes qw(gettimeofday tv_interval);
use strict;

print "Starting Benchmark\n";
my $startTime = [gettimeofday]; 
for (my $ii = 0; $ii < 10; $ii++)
{
  print "$ii \n"; 
  system('./multi-lookup input/names1.txt  input/names2.txt input/names3.txt input/names4.txt input/names5.txt output.txt ');
}
print "\n";
print tv_interval($startTime)."\n"; 