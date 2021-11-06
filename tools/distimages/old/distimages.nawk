#!/bin/nawk -f
# auxilary processing script; do not use this directly; only for use by
# distimages script.

BEGIN {
	vastro = "/vlb2/astrolabs/"
	lastro = "/lucifer/astrolabs/"
	castro = "/cassius/astrolabs/"
	exastro = "/export/data/"
}
/^m.*\.ft[sh]$/	{
    system ("mv " $1 " " castro "modern/" $1)
}
/^g.*\.ft[sh]$/	{
    system ("mv " $1 " " castro "general/" substr($1,2,1) "/" $1)
}
/^f.*\.ft[sh]$/	{
    system ("mv " $1 " " vastro "faculty/" $1)
}
/^u.*\.ft[sh]$/	{
    system ("mv " $1 " " exastro "ugrad/" $1)
}
/^w.*\.ft[sh]$/	{
    system ("mv " $1 " " castro "west/" $1)
}
/^xs.*\.ft[sh]$/	{
    system ("mv " $1 " " exastro "snrsrch/xs" substr($1,3,1) "/" $1)
}
/^xva.*\.ft[sh]$/       {
    system ("mv " $1 " " exastro "xva/" $1)
}
/^ccv.*\.ft[sh]$/	{
    system ("mv " $1 " " exastro "catvar/newdate/" $1)
}
/^xeb.*\.ft[sh]$/ {
    system ("mv " $1 " " exastro "xeb/"  $1)
}
/^[ij].*\.ft[sh]$/	{
    system ("mv " $1 " /vlb/ftp/pub/remote/images/" $1)
}
