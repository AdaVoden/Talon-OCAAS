#!/usr/bin/perl
# main IRO weather page

# no stdout buffering
$| = 1;

# first some basic html
print "Content-type: text/html\n\n";
print <<xEOFx;
<HTML>
<HEAD>
<TITLE>IRO Weather</TITLE>
</HEAD>
<body bgcolor="#ffffff">
<font color="red" size=6>
Weather data at the Iowa Robotic Observatory,
Winer Mobile Observatory, Sonoita AZ, USA.
</font>
<p>
<img src="http://www-astro.physics.uiowa.edu/images/main_line.gif">
<P>
Below are several graphs showing weather data for the last several days.
The data are updated each hour. The abscissa is two digits for
the year followed by three digits for the number of days since Jan 1.
Times are local as per the timezone indicated.
The ordinate depends on the quantity being displayed as per the key.  Click on the image for better resolution.
<P>

The following are plots of the temperature and pressure (left) and temperature and humidity (right).<p>
<a href="irowx1.pl" border="0"><IMG SRC="/cgi-bin/irowx/irowx1.pl" WIDTH = "200"></a>
<a href="irowx2.pl" border="0"><IMG SRC="/cgi-bin/irowx/irowx2.pl" WIDTH = "200"></a>
<p>
The following are plots of the rainfall (left) and windspeed (right).<p>

<a href="irowx3.pl" border="0"><IMG SRC="/cgi-bin/irowx/irowx3.pl" WIDTH = "200"></a>
<a href="irowx4.pl" border="0"><IMG SRC="/cgi-bin/irowx/irowx4.pl" WIDTH = "200"></a>
<p>
Here is a composite plot.<p>
<a href="irowx.pl" border="0"><IMG SRC="/cgi-bin/irowx/irowx.pl" WIDTH = "100"></a>
xEOFx

# now show the latest conditions
$dsgn = "&#176;";
$_ = `tail -1 /usr/local/telescope/archive/logs/wx.log`;
chop();
($jd,$ws,$wd,$temp,$hum,$pres,$rain) = split();
print "<P><H2>Latest Conditions as of ";
system ("/usr/local/telescope/bin/jd $jd");
print "</H2>\n";
print "<table>\n";
$degf = sprintf ("%.1f", 9*$temp/5 + 32);
print "<tr><td>Temperature:</td><td>$temp ${dsgn}C ($degf ${dsgn}F)</td></tr>\n";
print "<tr><td>Humidity:</td><td>$hum percent relative</td></tr>\n";
$inHg = sprintf ("%.2f", $pres/33.86);
print "<tr><td>Pressure:</td><td>$pres mBar ($inHg\")</td></tr>\n";
$mph = sprintf ("%.0f", $ws/1.609);
print "<tr><td>Wind:</td> <td>$ws kph ($mph mph) from $wd$dsgn E of N</td></tr>\n";
$inr = sprintf ("%.2f", $rain/25.4);
print "<tr><td>Rain:</td><td>$rain mm ($inr inches) since last reset</td></tr>\n";
print "</table>";

# finish up
print <<xEOFx2;
</BODY>
</HTML>
xEOFx2
