#######################################################################
## Makefile.am for alice xrdcp apmon library
##
##
## Version info: $Id: Makefile.am,v 1.5 2006/07/24 07:38:57 dfeich Exp $
## Checked in by $Author: dfeich $
#######################################################################

ACLOCAL_AMFLAGS = -I m4

lib_LTLIBRARIES = libXrdCpMonitorClient.la
AM_CPPFLAGS = -I$(XROOTD_LOCATION)/include/xrootd -I$(APMON_LOCATION)/include

###############################################

libXrdCpMonitorClient_la_SOURCES = XrdCpapMon.cc

#nodist_libXrdClientMonIntf_la_SOURCES = $(OFSSOURCES)

libXrdCpMonitorClient_la_LIBADD = -L$(APMON_LOCATION)/lib64/ -ldl -lpthread -lapmoncpp
#libXrdCpMonitorClient_la_LDFLAGS = -static -static-libgcc

EXTRA_DIST = configure.ac bootstrap.sh


