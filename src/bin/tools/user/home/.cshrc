setenv TELHOME /usr/local/telescope

set path = ($TELHOME/bin $path)
set cdpath = ($HOME $TELHOME $TELHOME/archive)
set notify
set prompt = `hostname -s`': '
set history=100
set savehist = 50

alias c		clear
alias hi 	history
alias lf	"ls -F"
alias psa	ps axf
alias psme	'ps aux \!* | grep $LOGNAME'
alias vime	'psme | grep vi'
alias vis	vi "+/\!*" '`grep -l \!* *.[ch]`'
alias vit	'vi -t \!*'

setenv LESS -eq
setenv PAGER less
setenv XUSERFILESEARCHPATH %N:$HOME/APPLRESDIR/%N

if (-e telescope/bin/sourceme) source telescope/bin/sourceme
