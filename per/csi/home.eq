! CSI development platform as alt-az
! Fill in the rough initial values for the telescope orientation and limit
! switch positions once. Then since the last entry for a given name is all that
! counts the Limit and Calibrate Axes procedures from xtelescope will append to
! this file and thereby override these values.

! initial guesses
HT = 0		! HA of scope pole, rads
DT = 1.57       ! Dec of scope pole, rads
XP = 3.14		! "X" angle from home to celestial pole,
                !   rads ccw as seen from tel pole
YC = 1.57         ! "Y" angle from scope's equator to home, rads +N
NP = 0.0        ! nonperpendicularity of axes, rads
R0 = 0.0        ! field rotator offset, if applicable


RNEGLIM = 0
RPOSLIM = 0
DPOSLIM = 2.8
DNEGLIM = -2.8
HPOSLIM = 3.0
HNEGLIM = -3.0
