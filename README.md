# li-panel

li-panel is core of desktop environment that can run under any compositor (being it Wayland or x11 - currently the support is limited) and takes care of all desktop widgets like backgrounds, panels, ~~notifications, volume/brightness popups,~~ etc. 

<!-- TODO: screenshot -->

## Usage

The whole li-panel is based on number of plugins and plugin runner (which only runs the plugins with their configs).
To configure the li-panel, update the `li-panel.cfg` file (should be in the same directory as the binary).
The format should be as following:
```
<plugin name>{
ID: <unique ID>
PLACEMENT: <container ID | WINDOW | NULL> <optional CSS>
option: value
}

<plugin name>{
...
}

```
Example:
```
panel{
ID: panel0
PLACEMENT: WINDOW -1 0 0 0 -1 60
}

run{
ID: run0
PLACEMENT: panel0
}

windowlist{
ID: wl0
PLACEMENT: panel0 flex-grow:9999;
}

datetime{
ID: datetime0
PLACEMENT: panel0 width:120px;
format: %H:%M
}

```


## Install

**Build from source:**
```
sudo pacman -S wayland wlr-protocols
git clone https://github.com/mi4code/li-panel
cd li-panel
mkdir build
cd build
make -f ../Makefile
```


## Applets
Applet plugin interface is described in [example applet source](./Applet_example.cc).


## TODOs
 - [ ] multi-screen support (default config for all screens, support for automatic handling of new screens / screen disconection)
 - [ ] conditional applet loading
 - [ ] titlebar and windowlist window menu
 - [ ] system tray
 - [ ] universally stylable applets (with HTML+CSS guidelines)

