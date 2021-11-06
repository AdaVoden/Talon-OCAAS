DIR = $(TELHOME)/archive/config

pass3greek: all

all: 				\
	$(DIR)/XEphem		\
	$(DIR)/ap7b.ini		\
	$(DIR)/blink.idx	\
	$(DIR)/boot.cfg		\
	$(DIR)/camera.cfg	\
	$(DIR)/csimc.cfg	\
	$(DIR)/dome.cfg		\
	$(DIR)/filter.cfg	\
	$(DIR)/focus.cfg	\
	$(DIR)/gpsd.cfg		\
	$(DIR)/home.cfg		\
	$(DIR)/logo.xpm		\
	$(DIR)/telescoped.cfg	\
	$(DIR)/telsched.cfg	\
	$(DIR)/wx.cfg		\
	$(DIR)/basic.cmc	\
	$(DIR)/find.cmc		\
	$(DIR)/lights.cmc	\
	$(DIR)/greekHA.cmc	\
	$(DIR)/greekDec.cmc	\
	$(DIR)/greekFocus.cmc

$(DIR)/XEphem: XEphem
	rm -f $(DIR)/XEphem
	cp XEphem $(DIR)

$(DIR)/ap7b.ini: ap7b.ini
	rm -f $(DIR)/ap7b.ini
	cp ap7b.ini $(DIR)

$(DIR)/blink.idx: blink.idx
	rm -f $(DIR)/blink.idx
	cp blink.idx $(DIR)

$(DIR)/boot.cfg: boot.cfg
	rm -f $(DIR)/boot.cfg
	cp boot.cfg $(DIR)

$(DIR)/camera.cfg: camera.cfg
	rm -f $(DIR)/camera.cfg
	cp camera.cfg $(DIR)

$(DIR)/csimc.cfg: camera.cfg
	rm -f $(DIR)/csimc.cfg
	cp csimc.cfg $(DIR)

$(DIR)/dome.cfg: dome.cfg
	rm -f $(DIR)/dome.cfg
	cp dome.cfg $(DIR)

$(DIR)/filter.cfg: filter.cfg
	rm -f $(DIR)/filter.cfg
	cp filter.cfg $(DIR)
	chmod ug+w $(DIR)/filter.cfg

$(DIR)/focus.cfg: focus.cfg
	rm -f $(DIR)/focus.cfg
	cp focus.cfg $(DIR)
	chmod ug+w $(DIR)/focus.cfg

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

$(DIR)/basic.cmc: basic.cmc
	rm -f $(DIR)/basic.cmc
	cp basic.cmc $(DIR)

$(DIR)/find.cmc: find.cmc
	rm -f $(DIR)/find.cmc
	cp find.cmc $(DIR)

$(DIR)/lights.cmc: lights.cmc
	rm -f $(DIR)/lights.cmc
	cp lights.cmc $(DIR)

$(DIR)/greekHA.cmc: greekHA.cmc
	rm -f $(DIR)/greekHA.cmc
	cp greekHA.cmc $(DIR)

$(DIR)/greekDec.cmc: greekDec.cmc
	rm -f $(DIR)/greekDec.cmc
	cp greekDec.cmc $(DIR)

$(DIR)/greekFocus.cmc: greekFocus.cmc
	rm -f $(DIR)/greekFocus.cmc
	cp greekFocus.cmc $(DIR)

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:22 $ $Revision: 1.1.1.1 $ $Name:  $
