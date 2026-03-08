#define PANEL_APPLET
#include "./Applet.h"
#include <limits>
#include <ctime>
#include <map>


// global variables can go here (these are shared across all instances of the plugin - even if dlopen was called each time instance was created)
int instance_count;
std::map<void*,int> instances;


// exported actions of the applet (that can be used for automation, etc.)
const std::map<std::string, void(*)(std::string)> Applet::commands () {
	return { {"custom_command", [](std::string)->void{/*perform action here*/}}, };
}

// initialize default values (dont include global=uppercase common keys)
const std::map<std::string,std::string> Applet::settings_default () {
	return { {"text","example"}, };
}


// gets ran when applet instance is added, it is guaranteed that 'settings' are set and 'window' is either valid with element matching 'ID' or 'nullptr'
void Applet::load () {
	
	// applet is not supposed to be anywhere or is not expected to need window (so quit)
	if (window == nullptr) return;
	
	// keep count of instances for display
	instance_count++;
	instances[this] = instance_count;
	
	// add content to applet (if you can, please avoid changing other html than applets own)
	window->html_element("#"+settings["ID"], "innerText", settings["text"]);

	// in case you want the applet to act as container for other applets  create inside element matching '#<id>.container'
	// window->call_js("document.querySelector('#" + settings["ID"] + "').classList.add('container')");
	
	// add click action
	window->html_element(
		"#" + settings["ID"], 
		"onclick", 
		window->call_native( [this](std::vector<std::string> args) -> void { window->html_element("#"+settings["ID"],"innerText","'clicked (instance "+std::to_string(instances[this])+")'"); } ) 
	);
}

void Applet::reload () {
	// gets ran when settings have changed (not needed when applet gets updated regularily)
	
	// add text into the applet area
	window->call_js(std::string() + "document.querySelector('#" + settings["ID"] + "').innerText = '" + settings["text"] + "';");
	
	// you can use 'unload()' + 'load()' if you dont need to preserve state
	// unload();
	// load();
}

void Applet::unload () {
	// cleanup before the applet gets removed (no need to remove html elements, just kill all child processes, relese memory, clean js stuff)

	// not much to do in this case
	instances.erase(this);
}


std::time_t Applet::update () {
	// runs every few seconds to allow updates of the applet (next run time is determined by ctime seconds returned)
	// ideally avoid disk reads or anything resource-heavy here
	
	// update every 10 seconds
	return std::time(nullptr)+10;
	
	// in case you dont need to update, return max
	// return std::numeric_limits<std::time_t>::max();
	
	// if you need to run update more often than every second, return current time (this currently means every 35ms but can change in the future, use with care) 
	// return std::time(nullptr);
}
