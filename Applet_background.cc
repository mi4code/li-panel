#define PANEL_APPLET
#include "./Applet.h"
#include <limits>

const std::map<std::string, void(*)(std::string)> Applet::commands () {
	return {};
}

const std::map<std::string,std::string> Applet::settings_default () {
	return {{"display","0"},{"file",""}};
}


void Applet::load() {
	if (settings["PLACEMENT"].starts_with("WINDOW")) {
		window->load_file(HUI::filepath(settings["file"]).cpp_str());
		
		HUI::WindowControls windowctl(window->backend_object(), window->window_handle());
		windowctl.set_type(WT_DESKTOP_COMPONENT);
		windowctl.set_layer(WL_BACKGROUND);
		windowctl.set_geometry({.state = 0, .monitor = std::stoi(settings["display"]), .width = -1, .height = -1, .left = 0, .top = 0, .right = 0, .bottom = 0});
		windowctl.set_exclusive_zone(-2); // make the background go under the panel
	}
}

void Applet::reload () {
	load();
}

void Applet::unload () {
}


std::time_t Applet::update () {
	// never update
	return std::numeric_limits<std::time_t>::max();
}
