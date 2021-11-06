#!/usr/bin/perl

# gate    hi
system("lpio -1 2");
#toggle gate
while(1) {
	print "setting bit 1 to 0\n";
	system("lpio -0 1");
        mssleep(1000);

	print "setting bit 1 to 1\n";
	system("lpio -1 1");
	mssleep(1000);
};


exit (0);

sub mssleep
{
	my $ms = $_[0];
	select (undef, undef, undef, $ms/1000.);
}
