SET(MAPVIEWX_PLUGIN_HEADERS groundmotion.h)
SET(MAPVIEWX_PLUGIN_SOURCES groundmotion.cpp)

# Install the headers in install/seiscomp/plugins/scmvx
SC_ADD_LIBRARY(MAPVIEWX_PLUGIN mvx)
SC_LIB_INSTALL_HEADERS(MAPVIEWX_PLUGIN seiscomp/plugins/mvx)
SC_LIB_LINK_LIBRARIES_INTERNAL(mvx qt)
