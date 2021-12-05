Talon (OCAAS)
================================================================================

Attempt to build legacy Talon Observatory Control software on modern systems

Current version builds and installs and can operate a fully functional research telescope.

**NOTE:** All camera-related code has been excised due to lack of necessity.

Original source: https://sourceforge.net/projects/observatory/

Included latest version of manual that can be found online.

I wrote none of the source code. Use at your own risk. Here there be dragons.



Build
-------------------------------------------------------------------------------

### Debian-based distros

Tested on **Ubuntu LTS 20.04**

install required development packages:

  `sudo apt install libmotif-dev libxpm-dev libxext-dev libxmu-dev libdb-dev`
  
Download the repository and into a directory, enter it in the commandline and run

  `mkdir build`
  
  `cd build`
  
  `cmake ../.`
  
  `cpack -G DEB`

this will create an installable .deb file for you to use.

Install
---------

Using the .deb file:

  `sudo apt install ./talon-ocaas_0.1_amd64.deb`
  
This will create a series of folders under /usr/local/telescope. The ***comm*** folder is used for internal communication, ***bin*** contains the binaries, ***lib*** contains the libraries and ***archive*** contains logs and configuration.

Set the OCAAS software to automatically start via the commands:

  `sudo systemctl enable weather`
  
  `sudo systemctl enable telescope`

Usage
------

When connected to a telescope and weather station, you can set the software's required daemons (csicmd, wxd, telescoped) to run via the commands
  
  `sudo systemctl start weather`
  
  `sudo systemctl start telescope`
  
  `xobs&`
 
Where the ampersand launches the XOBS GUI software in a new process.



