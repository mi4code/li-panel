#define PANEL_APPLET
#include "./Applet.h"
#include <limits>


const std::map<std::string, void(*)(std::string)> Applet::commands () {
	return {};
}

const std::map<std::string,std::string> Applet::settings_default () {
	return {};
}


void Applet::load() {
	if (window == NULL) return;
	
	const auto html = "`\
	<style>\
		#"+settings["ID"]+" {padding: 15px;}\
		#"+settings["ID"]+" input {background-color: rgba(0,0,0,0); color: white; border: 2px solid white;}\
		#"+settings["ID"]+" input:hover, #"+settings["ID"]+" input:focus {background-color: rgba(20,20,20,1); border: 2px solid red;}\
	</style>\
	<input type=\"text\" onchange=\"command_run(this.value); this.value = \\'\\';\" style=\"width: 120px; height: calc(100% - 2*15px - 2*2px);\" class=\"hui_unstyled\">`";

	window->html_element("#"+settings["ID"], "innerHTML", html);
	window->call_js(std::string()+"command_run = "+window->call_native([](std::vector<std::string> args) -> void { 
		system((args[0]+" &").c_str());
	})+";");
}

void Applet::reload () {
	// nothing to be reloaded (no settings)
}

void Applet::unload () {
	// TODO: clear 'command_run'
}


std::time_t Applet::update () {
	// never update
	return std::numeric_limits<std::time_t>::max();
}