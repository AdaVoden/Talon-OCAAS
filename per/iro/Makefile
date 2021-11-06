DIR = $(TELHOME)/archive/config

pass3iro: all

all: 				\
	$(DIR)/ap8.ini		\
	$(DIR)/XEphem		\
	$(DIR)/blink.idx	\
	$(DIR)/boot.cfg		\
	$(DIR)/camera.cfg	\
	$(DIR)/dome.cfg		\
	$(DIR)/filter.cfg	\
	$(DIR)/focus.cfg	\
	$(DIR)/gpsd.cfg		\
	$(DIR)/home.cfg		\
	$(DIR)/logo.xpm		\
	$(DIR)/telescoped.cfg	\
	$(DIR)/telsched.cfg	\
	$(DIR)/wx.cfg

$(DIR)/ap8.ini: ap8.ini
	rm -f $(DIR)/ap8.ini
	cp ap8.ini $(DIR)

$(DIR)/XEphem: XEphem
	rm -f $(DIR)/XEphem
	cp XEphem $(DIR)

$(DIR)/blink.idx: blink.idx
	rm -f $(DIR)/blink.idx
	cp blink.idx $(DIR)

$(DIR)/boot.cfg: boot.cfg
	rm -f $(DIR)/boot.cfg
	cp boot.cfg $(DIR)

$(DIR)/camera.cfg: camera.cfg
	rm -f $(DIR)/camera.cfg
	cp camera.cfg $(DIR)

$(DIR)/dome.cfg: dome.cfg
	rm -f $(DIR)/dome.cfg
	cp dome.cfg $(DIR)

$(DIR)/filter.cfg: filter.cfg
	rm -f $(DIR)/filter.cfg
	cp filter.cfg $(DIR)

$(DIR)/focus.cfg: focus.cfg
	rm -f $(DIR)/focus.cfg
	cp focus.cfg $(DIR)

$(DIR)/gpsd.cfg: gpsd.cfg
	rm -f $(DIR)/gpsd.cfg
	cp gpsd.cfg $(DIR)

$(DIR)/home.cfg: home.cfg
	rm -f $(DIR)/home.cfg
	cp home.cfg $(DIR)/home.cfg
	chmod ug+w $(DIR)/home.cfg

$(DIR)/logo.xpm: logo.xpm
	rm -f $(DIR)/logo.xpm
	cp logo.xpm $(DIR)

$(DIR)/telescoped.cfg: telescoped.cfg
	rm -f $(DIR)/telescoped.cfg
	cp telescoped.cfg $(DIR)

$(DIR)/telsched.cfg: telsched.cfg
	rm -f $(DIR)/telsched.cfg
	cp telsched.cfg $(DIR)
	chmod ug+w $(DIR)/telsched.cfg

$(DIR)/wx.cfg: wx.cfg
	rm -f $(DIR)/wx.cfg
	cp wx.cfg $(DIR)

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:23 $ $Revision: 1.1.1.1 $ $Name:  $
