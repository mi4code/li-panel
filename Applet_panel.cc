#define PANEL_APPLET
#include "./Applet.h"
#include <limits>
#include <ctime>
#include <sstream>


const std::map<std::string, void(*)(std::string)> Applet::commands () {
	return {};
}

const std::map<std::string,std::string> Applet::settings_default () {
	return {{"focus_mode","window"}};
}


void Applet::load () {
	
	// setup window props
	if (settings["PLACEMENT"].starts_with("WINDOW")) {

		std::vector<std::string> gm;
		std::stringstream s(settings["PLACEMENT"]);
		std::string v;
		while (s >> v) gm.push_back(v);
	
		HUI::WindowControls windowctl(window->backend_object(), window->window_handle());
		windowctl.set_type(WT_DESKTOP_COMPONENT);
		windowctl.set_layer(WL_TOP);
		windowctl.set_geometry({
			.state = 0,
			.monitor = -1,
			.width = std::stoi(gm[5]),
			.height = std::stoi(gm[6]),
			.left = std::stoi(gm[2]),
			.top = std::stoi(gm[1]),
			.right = std::stoi(gm[3]), 
			.bottom = std::stoi(gm[4])
			});
		if (settings["focus_mode"] == "never") windowctl.set_input_mode_keyboard(WIM_NEVER);
		else if (settings["focus_mode"] == "always") windowctl.set_input_mode_keyboard(WIM_NEVER);
		else windowctl.set_input_mode_keyboard(WIM_AUTO_WINDOW);
		windowctl.set_exclusive_zone(-2);
		
		if (settings["focus_mode"] == "inputs") {
			// force unfocus panel window if clicked (with exception for inputs)
			window->call_js(std::string() + "document.addEventListener('pointerdown', e => { if (!e.target.closest('input,textarea,select,[contenteditable]')) (" + window->call_native([this](std::vector<std::string>){
				if (settings["focus_mode"] != "inputs") return; // TODO: this eventlistener stays after config update but it shouldnt
				HUI::WindowControls(window->backend_object(), window->window_handle()).set_input_mode_keyboard(WIM_NEVER);
				next_update = std::time(nullptr)+1;
			}) + ")(); }, true);");
		}
		
	}
	else {
		// TODO: exclusive zone for panel without own window
	}
	
	
	// add text into the applet area
	window->call_js(std::string() + "document.querySelector('#" + settings["ID"] + "').innerHTML = `" + R"(
<style>
	*:has(> #)" + settings["ID"] + R"() {
	  container-type: size;
	}

	#)" + settings["ID"] + R"( {
	  display: flex;
	  
	  background-color: rgba(0,0,0,0.5);
	  
	  container-type: size;
	}

	@container (aspect-ratio > 1) {
	  #)" + settings["ID"] + R"( {
		flex-direction: row;
	  }
	  #)" + settings["ID"] + R"( > .applet {
		height: 100% !important;
		width: auto;
	  }
	}

	@container (aspect-ratio < 1) {
	  #)" + settings["ID"] + R"( {
		flex-direction: column;
	  }
	  #)" + settings["ID"] + R"( > .applet {
		width: 100% !important;
		height: auto;
	  }
	}
	
	#)" + settings["ID"] + R"( > .applet {
	  margin: 0;
	  
	  color: white;
	  background-color: rgba(0,0,0,0);
	  /*border: 4px solid red;*/
	}
	
	#)" + settings["ID"] + R"( > .applet:hover, #)" + settings["ID"] + R"( > .applet:hover svg {
	  background-color: rgba(0,0,0,0);
	  color: red;
	}
</style>
	)" + "`;");
	
	// hint that this is applet area 
	window->call_js(std::string() + "document.querySelector('#" + settings["ID"] + "').classList.add('container');");
	
}

void Applet::reload () {
	load();
}

void Applet::unload () {
	// TODO: clean exclusive zone
}


std::time_t Applet::update () {
	// restore keyboard mode after force unfocus
	if (settings["focus_mode"] == "inputs") HUI::WindowControls(window->backend_object(), window->window_handle()).set_input_mode_keyboard(WIM_AUTO_WINDOW);
	
	// never updated
	return std::numeric_limits<std::time_t>::max();
}
