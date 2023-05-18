/*
  Copyleft (ɔ) 2009 Kernc
  This program is free software. It comes with absolutely no warranty whatsoever.
  See COPYING for further information.
  
  Project homepage: https://github.com/kernc/chrolog_iohook
*/

#include <cstdlib>
#include <unistd.h>

int main() {
  setuid(0);
  exit(system(SYS_CONF_DIR "/chrolog_iohook-start.sh"));  // SYS_CONF_DIR defined in CXXFLAGS in Makefile.am
}
