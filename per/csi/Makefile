DIR = $(TELHOME)/archive/config

pass3csi: all

all: 				\
	$(DIR)/ap1.ini		\
	$(DIR)/XEphem		\
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
	$(DIR)/ip.cfg		\
	$(DIR)/basic.cmc	\
	$(DIR)/find.cmc		\
	$(DIR)/lights.cmc	\
	$(DIR)/loretta0.cmc	\
	$(DIR)/loretta1.cmc

$(DIR)/ap1.ini: ap1.ini
	rm -f $(DIR)/ap1.ini
	cp ap1.ini $(DIR)

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

$(DIR)/csimc.cfg: camera.cfg
	rm -f $(DIR)/csimc.cfg
	cp csimc.cfg $(DIR)

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

$(DIR)/ip.cfg: ip.cfg
	rm -f $(DIR)/ip.cfg
	cp ip.cfg $(DIR)/ip.cfg

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

$(DIR)/loretta0.cmc: loretta0.cmc
	rm -f $(DIR)/loretta0.cmc
	cp loretta0.cmc $(DIR)

$(DIR)/loretta1.cmc: loretta1.cmc
	rm -f $(DIR)/loretta1.cmc
	cp loretta1.cmc $(DIR)

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:21 $ $Revision: 1.1.1.1 $ $Name:  $
