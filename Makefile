.PHONY: all plugins plugins-wlr core

all: core plugins plugins-wlr

core:
	g++ -w -std=c++20 -O2 -ldl -o li_panel ../runner.cc -I../../HUI -L. -lHUI
	
plugins:
	LIST="background custom datetime example panel run"; \
	for f in $$LIST; do  g++ -w -std=c++20 -O2 -fPIC -shared -o Applet_$$f.so ../Applet_$$f.cc -I../../HUI -L. -lHUI  ; done

plugins-wlr:
	wayland-scanner client-header /usr/share/wlr-protocols/unstable/wlr-foreign-toplevel-management-unstable-v1.xml wlr-foreign-toplevel-management-unstable-v1-client-protocol.h
	wayland-scanner private-code  /usr/share/wlr-protocols/unstable/wlr-foreign-toplevel-management-unstable-v1.xml wlr-foreign-toplevel-management-unstable-v1-client-protocol.c
	g++ -w -std=c++20 -O2 -fPIC -shared  -o "Applet_windowlist.so" ../Applet_windowlist.cc -I. -I../../HUI -L. -lHUI `pkg-config --cflags --libs wayland-client`
