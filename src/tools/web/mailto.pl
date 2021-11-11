#!/opt/local/bin/perl
#
# Doug's WWW Mail Gateway 2.2
# 5/95
# All material here is Copyright 1995 Doug Stevenson.
#
# Use this script as a front end to mail in your HTML.  Not every browser
# supports the mailto: URLs, so this is the next best thing.  If you
# use this script, please leave credits to myself intact!  :)  You can
# modify it all you want, though.
#
# Documentation at:
#    http://www-bprc.mps.ohio-state.edu/mailto/mailto_info.html
#
# Configurable items are just below.  Also pay special attention to
# GET method arguments that this script accepts to specify defaults
# for some fields.
#
# I didn't exactly follow the RFCs on mail headers when I wrote this,
# so please send all flames my way if it breaks your mail client!!
# Also, you'll need cgi-lib.pl for the GET and POST parsing.  I use
# version 1.7.
#
# Requires cgi-lib.pl which can be found at
#    http://www.bio.cam.ac.uk/web/form.html
#
# PLEASE: Use this script freely, but leave credits to myself!!  It's
#         common decency!
#
########
#
# Changes from 1.1 to 1.2:
#
# A common modification to the script for others to make was to allow
# only a certain few mail addresses to be sent to.  I changed the WWW
# Mail Gateway to allow only those mail addresses in the list @addrs
# to be mailed to - they are placed in a HTML <SELECT> list, with either
# the selected option being either the first one or the one that matches
# the "to" CGI variable.  Thanks to Mathias Koerber
# <Mathias.Koerber@swi.com.sg> for this suggestion.
#
# Also made one minor fix.
#
########
#
# Changes from 1.2 to 1.3:
#
# Enhancing the enhancements from 1.2.  You can now specify a real name
# or some kind of identifier to go with the real mail address.  This
# infomation gets put in the %addrs associative array, either explicitly
# defined, or read from a file.  Read the information HTML for instructions
# on how to set this up.  Also, real mail addresses may hidden from the
# user.  Undefine or set to zero the variable $expose_address below.
#
########
#
# Changes from 1.3 to 1.4
#
# The next URL to be fetched after the mail is sent can be specified with
# the cgi varaible 'nexturl'.
#
# Fixed some stupid HTML mistake.
#
# Force user to enter something for the username on 'Your Email:' tag,
# if identd didn't get a username.
#
# Added Cc: field, only when %addrs is not being used.
#
########
#
# Quickie patch to 1.41
#
# Added <PRE>formatted part to header entry to make it look nice and fixed a
# typo.
#
########
#
# Version 2.0 changes
#
# ALL cgi varaibles (except those reserved for mail info) are logged
# at then end of the mail received.  You can put forms, hidden data,
# or whatever you want, and the info for each variable will get logged.
#
# Cleaned up a lot of spare code.
#
# IP addresses are now correctly logged instead of just hostnames.
#
# Made source retrieval optional.
#
########
#
# Changes from 2.0 to 2.1
#
# Fixed stupid HTML error for an obscure case.  Probably never noticed.
#
# Reported keys are no longer reported in an apparently random order; they
# are listed in the order they were received.  That was a function of perl
# hashes...changed to a list operation instead.
#
########
#
# Changes from 2.1 to 2.2
#
# Added all kinds of robust error checking and reporting.  Should be
# easier to diagnose problems from the user end.
#
# New suggested sendmail flag -oi to keep sendmail from ending mail
# input on line containing . only.
#
# Added support for setting the "real" From address in the first line
# of the mail header using the -f sendmail switch.  This may or may not
# be what you want, depending on the application of the script.  This is
# useful for listservers that use that information for identification
# purposes or whatever.  This is NOT useful if you're concerned about
# the security of your script for public usage.  Your mileage will vary,
# please read the sendmail manual about the -f switch.
#    Thanks to Jeff Lawrence (jlaw@irus.rri.uwo.ca) for figuring this
#    one out.
#
########
#
# Doug Stevenson
# doug+@osu.edu

######################
# Configurable options
######################

# whether or not to actually allow mail to be sent -- for testing purposes
$active = 1;

# Logging flag.  Logs on POST method when mail is sent.
$logging = 0;
$logfile = '/export/brutus/Web/cgi-bin/logs/mailto.log';

# Physical script location.  Define ONLY if you wish to make your version
# of this source code available with GET method and the suffix '?source'
# on the url.
$script_loc = '/export/brutus/Web/cgi-bin/mailto.pl';

# physical location of your cgi-lib.pl
$cgi_lib = 'cgi-lib-97.pl';

# http script location
$script_http = 'http://www-astro.physics.uiowa.edu/cgi-bin/mailto.pl';

# Path to sendmail and its flags.  Use the first commented version and
# define $listserver = 1if you want the gateway to be used for listserver
# subscriptions -- the -f switch might be neccesary to get this to work
# correctly.
#
# sendmail options:
#    -n  no aliasing
#    -t  read message for "To:"
#    -oi don't terminate message on line containing '.' alone
#$sendmail = "/usr/lib/sendmail -t -n -oi -f";  $listserver = 1;
$sendmail = "/usr/lib/sendmail -t -oi";

# set to 1 if you want the real addresses to be exposed from %addrs
#$expose_address = 1;

# Uncomment one of the below chunks of code to implement restricted mail

# List of address to allow ONLY - gets put in a HTML SELECT type menu.
#
#%addrs = ("Doug - main address", "doug+@osu.edu",
#	  "Doug at BPRC", "doug@polarmet1.mps.ohio-state.edu",
#	  "Doug at CIS", "stevenso@cis.ohio-state.edu",
#	  "Doug at the calc lab", "dstevens@mathserver.mps.ohio-state.edu",
#	  "Doug at Magnus", "dmsteven@magnus.acs.ohio-state.edu");

# If you don't want the actual mail addresses to be visible by people
# who view source, or you don't want to mess with the source, read them
# from $mailto_addrs:
#
#$mailto_addrs = '/usr/local/WWW/etc/mailto_addrs';
#open(ADDRS,$mailto_addrs);
#while(<ADDRS>) {
#    ($name,$address) = /^(.+)[ \t]+([^ ]+)\n$/;
#    $name =~ s/[ \t]*$//;
#    $addrs{$name} = $address;
#}

# version
$version = '2.2';

#############################
# end of configurable options
#############################


##########################
# source is self-contained
##########################

if ($ENV{'QUERY_STRING'} eq 'source' && defined($script_loc)) {
    print "Content-Type: text/plain\n\n";
    open(SOURCE, $script_loc) ||
	&InternalError('Could not open file containing source code');
    print <SOURCE>;
    close(SOURCE);
    exit(0);
}

require $cgi_lib;
&ReadParse();

#########################################################################
# method GET implies that we want to be given a FORM to fill out for mail
#########################################################################

if ($ENV{'REQUEST_METHOD'} eq 'GET') {
    # try to get as much info as possible for fields
    # To:     comes from $in{'to'}
    # Cc:     comes from $in{'cc'}
    # From:   comes from REMOTE_IDENT@REMOTE_HOST || $in{'from'} || REMOTE_USER
    # Subject: comes from $in{'sub'}
    # body comes from $in{'body'}

    $destaddr = $in{'to'};
    $cc = $in{'cc'};
    $subject = $in{'sub'};
    $body = $in{'body'};
    $nexturl = $in{'nexturl'};

    if ($in{'from'}) {
	$fromaddr = $in{'from'};
    }
    # this is for NetScape pre-1.0 beta users - probably obsolete code
    elsif ($ENV{'REMOTE_USER'}) {
	$fromaddr = $ENV{'REMOTE_USER'};
    }
    # this is for Lynx users, or any HTTP/1.0 client giving From header info
    elsif ($ENV{'HTTP_FROM'}) {
	$fromaddr = $ENV{'HTTP_FROM'};
    }
    # if all else fails, make a guess
    else {
	$fromaddr = "$ENV{'REMOTE_IDENT'}\@$ENV{'REMOTE_HOST'}";
    }

    # Convert multiple bodies (separated by \0 according to CGI spec)
    # into one big body
    $body =~ s/\0//;

    # Make a list of authorized addresses if %addrs exists.
    if (%addrs) {
	$selections = '<SELECT NAME="to">';
	foreach (sort keys %addrs) {
	    if ($in{'to'} eq $addrs{$_}) {
		$selections .= "<OPTION SELECTED>$_";
	    }
	    else {
		$selections .= "<OPTION>$_";
	    }
	    if ($expose_address) {
		$selections .= " &lt;$addrs{$_}>";
	    }
	}
	$selections .= "</SELECT>\n";
    }

    # give them the form
    print &PrintHeader();
    print <<EOH;
<HTML><HEAD><TITLE>Doug\'s WWW Mail Gateway $version</TITLE></HEAD>
<BODY><H1><IMG SRC="http://www-bprc.mps.ohio-state.edu/pics/mail2.gif" ALT="">
The WWW Mail Gateway $version</H1>

<P>The <B>To</B>: field should contain the <B>full</B> E-mail address
that you want to mail to.  The <B>Your Email</B>: field needs to
contain your mail address so replies go to the right place.  Type your
message into the text area below. If the <B>To</B>: field is invalid,
or the mail bounces for some reason, you will receive notification
if <B>Your Email</B>: is set correctly.  <I>If <B>Your Email</B>:
is set incorrectly, all bounced mail will be sent to the bit bucket.</I></P>

<FORM ACTION="$script_http" METHOD=POST>
EOH
    ;
    print "<P><PRE>        <B>To</B>: ";

    # give the selections if set, or INPUT if not
    if ($selections) {
	print $selections;
    }
    else {
	print "<INPUT VALUE=\"$destaddr\" SIZE=40 NAME=\"to\">\n";
	print "        <B>Cc</B>: <INPUT VALUE=\"$cc\" SIZE=40 NAME=\"cc\">\n";
    }

    print <<EOH;
 <B>Your Name</B>: <INPUT VALUE="$fromname" SIZE=40 NAME="name">
<B>Your Email</B>: <INPUT VALUE="$fromaddr" SIZE=40 NAME="from">
   <B>Subject</B>: <INPUT VALUE="$subject" SIZE=40 NAME="sub"></PRE>
<INPUT TYPE="submit" VALUE="Send the mail">
<INPUT TYPE="reset" VALUE="Start over"><BR>
<TEXTAREA ROWS=20 COLS=60 NAME="body">$body</TEXTAREA><BR>
<INPUT TYPE="submit" VALUE="Send the mail">
<INPUT TYPE="reset" VALUE="Start over"><BR>
<INPUT TYPE="hidden" NAME="nexturl" VALUE="$nexturl"></P>
</FORM>
<HR>

<H2>Information about the WWW Mail Gateway</H2>
<H3><A HREF="http://www-bprc.mps.ohio-state.edu/mailto/mailto_info.html#about">
About the WWW Mail Gateway</A></H3>
<H3><A HREF="http://www-bprc.mps.ohio-state.edu/mailto/mailto_info.html#new">
New in version $version</A></H3>
<H3><A HREF="http://www-bprc.mps.ohio-state.edu/mailto/mailto_info.html#misuse">
Please report misuse!</A></H3>

<HR>
<ADDRESS><P><A HREF="/~doug/">Doug Stevenson: doug+\@osu.edu</A>
</P></ADDRESS>
</BODY></HTML>
EOH
    ;
}

#########################################################################
# Method POST implies that they already filled out the form and submitted
# it, and now it is to be processed.
#########################################################################

elsif ($ENV{'REQUEST_METHOD'} eq 'POST') {
    # get all the variables in their respective places
    $destaddr = $in{'to'};
    $cc       = $in{'cc'};
    $fromaddr = $in{'from'};
    $fromname = $in{'name'};
    $replyto  = $in{'from'};
    $sender   = $in{'from'};
    $errorsto = $in{'from'};
    $subject  = $in{'sub'};
    $body     = $in{'body'};
    $nexturl  = $in{'nexturl'};
    $realfrom = $ENV{'REMOTE_HOST'} ? $ENV{'REMOTE_HOST'}: $ENV{'REMOTE_ADDR'};

    # check to see if required inputs were filled - error if not
    unless ($destaddr && $fromaddr && $body && ($fromaddr =~ /^.+\@.+/)) {
	print <<EOH;
Content-type: text/html
Status: 400 Bad Request

<HTML><HEAD><TITLE>Mailto error</TITLE></HEAD>
<BODY><H1>Mailto error</H1>
<P>One or more of the following necessary pieces of information was missing
from your mail submission:
<UL>
<LI><B>To</B>:, the full mail address you wish to send mail to</LI>
<LI><B>Your Email</B>: your full email address</LI>
<LI><B>Body</B>: the text you wish to send</LI>
</UL>
Please go back and fill in the missing information.</P></BODY></HTML>
EOH
    exit(0);
    }

    # do some quick logging - you may opt to have more/different info written
    if ($logging) {
	open(MAILLOG,">>$logfile");
	print MAILLOG "$realfrom\n";
	close(MAILLOG);
    }

    # Log every CGI variable except for the ones reserved for mail info.
    # Valid vars go into @data.  Text output goes into $data and gets.
    # appended to the end of the mail.
    # First, get an ORDERED list of all cgi vars from @in to @keys
    for (0 .. $#in) {
	local($key) = split(/=/,$in[$_],2);
	$key =~ s/\+/ /g;
	$key =~ s/%(..)/pack("c",hex($1))/ge;
	push(@keys,$key);
    }
    # Now weed out the ones we want
    @reserved = ('to', 'cc', 'from', 'name', 'sub', 'body', 'nexturl');
    local(%mark);
    foreach (@reserved) { $mark{$_} = 1; }
    @data = grep(!$mark{$_}, @keys);
    foreach (@data) {
	$data .= "$_ -> $in{$_}\n";
    }

    # Convert multiple bodies (separated by \0 according to CGI spec)
    # into one big body
    $body =~ s/\0//;

    # now check to see if some joker changed the HTML to allow other
    # mail addresses besides the ones in %addrs, if applicable
    if (%addrs) {
	if (!scalar(grep($_." <$addrs{$_}>" eq $destaddr ||
			 $destaddr eq $_, keys(%addrs)))) {
	    print &PrintHeader();
	    print <<EOH;
<HTML><HEAD><TITLE>WWW Mail Gateway: Mail address not allowed</TITLE></HEAD>
<BODY>
<H1>Mail address not allowed</H1>
<P>The mail address you managed to submit, <B>$destaddr</B>, to this script is
not one of the pre-defined set of addresses that are allowed.  Go back and
try again.</P>
</BODY></HTML>
EOH
    ;
	    exit(0);
	}
    }

    # if we just received an alias, then convert that to an address
    $realaddr = $destaddr;
    if ($addrs{$destaddr}) {
	$realaddr = "$destaddr <$addrs{$destaddr}>";
    }

    # fork over the mail to sendmail and be done with it
    if ($active) {
	if ($listserver) {
	    open(MAIL,"| $sendmail$fromaddr") ||
		&InternalError('Could not fork sendmail with -f switch');
	}
	else {
	    open(MAIL,"| $sendmail") ||
		&InternalError('Could not fork sendmail with -f switch');
	}

	# only print Cc if we got one
	print MAIL "Cc: $cc\n" if $cc;
	print MAIL <<EOM;
From: $fromname <$fromaddr>
To: $realaddr
Reply-To: $replyto
Errors-To: $errorsto
Sender: $sender
Subject: $subject
X-Mail-Gateway: Doug\'s WWW Mail Gateway $version
X-Real-Host-From: $realfrom

$body

$data
EOM
    close(MAIL);
    }

    # give some short confirmation results
    #
    # if the cgi var 'nexturl' is given, give out the location, and let
    # the browser do the work.
    if ($nexturl) {
	print "Location: $nexturl\n\n";
    }
    # otherwise, give them the standard form.
    else {
	print &PrintHeader();
	print <<EOH;
<HTML><HEAD><TITLE>Mailto results</TITLE></HEAD>
<BODY><H1>Mailto results</H1>
<P>Mail sent to <B>$destaddr</B>:<BR><BR></P>
<PRE>
<B>Subject</B>: $subject
<B>From</B>: $fromname &lt;$fromaddr>

$body</PRE>
<HR>
<A HREF="$script_http">Back to the WWW Mailto Gateway</A>
</BODY></HTML>
EOH
    ;
    }
}				# end if METHOD=POST

#####################################
# What the heck are we doing here????
#####################################

else {
    print <<EOH;
<HTML><HEAD><TITLE>Mailto Gateway error</TITLE></HEAD>
<BODY><H1>Mailto Gateway error</H1>
<P>Somehow your browser generated a non POST/GET request method and it
got here.  You should get this fixed!!</P></BODY></HTML>
EOH
}

exit(0);


#
# Deal out error messages to the user.  Gets passed a string containing
# a description of the error
#
sub InternalError {
    local($errmsg) = @_;

    print &PrintHeader();
    print <<EOH;
Content-type: text/html
Status: 502 Bad Gateway

<HTML><HEAD><TITLE>Mailto Gateway Internal Error</TITLE></HEAD>
<BODY><H1>Mailto Gateway Internal Error</H1>
<P>Your mail failed to send for the following reason:<BR><BR>
<B>$errmesg</B></P></BODY></HTML>
EOH
    exit(0);
}


##
## end of mailto.pl
##

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: mailto.pl,v $ $Date: 2003/04/15 20:48:47 $ $Revision: 1.1.1.1 $ $Name:  $
