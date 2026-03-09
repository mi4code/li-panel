#define PANEL_APPLET
#include "./Applet.h"
#include <array>
#include <memory>
#include <cstdio> 


// TODO: no need to replace $VALUE with output since output can be appended in the command itself
std::string str_replace(std::string str, const std::string& old_str, const std::string& new_str) {
    size_t start_pos = str.find(old_str);
    if (start_pos == std::string::npos) return str;
    str.replace(start_pos, old_str.length(), new_str);
    return str;
}


const std::map<std::string, void(*)(std::string)> Applet::commands () {
	return {};
}

const std::map<std::string,std::string> Applet::settings_default () {
	return { {"mode","text"},{"content","$VALUE"},{"command","echo default"},{"update_interval","5"} }; 
}


void Applet::load() {

	if (settings["mode"] == "text")  settings["content"] = "<p>"+settings["content"]+"</p>";  // lets handle it as html mode // TODO: do we need it?
	
	window->html_element("#"+settings["ID"], "style.display", "'flex'");
	window->html_element("#"+settings["ID"], "style.alignItems", "'center'");
	
	for (auto s : settings) {
		if (s.first.starts_with("on")){
			std::string cmd = s.second;
			window->call_js( std::string() + "document.querySelector('#" + settings["ID"] + "').addEventListener('" + s.first.substr(2) + "'," + window->call_native([this, cmd](std::vector<std::string> args) -> void { system((cmd+" &").c_str()); update(); }) + ");");
		}
	}
	
	// custom events: onclick_l/r/m, onwheel_up/down/left/right
	window->call_js("document.querySelector('#"+settings["ID"]+"').addEventListener('click',e=>{ document.querySelector('#"+settings["ID"]+"').dispatchEvent(new CustomEvent('click_'+['l','m','r'][e.button],{detail:e})); });");
	window->call_js("document.querySelector('#"+settings["ID"]+"').addEventListener('wheel',e=>{ var d = e.deltaY!=0 ? (e.deltaY<0?'up':'down') : (e.deltaX<0?'left':'right'); document.querySelector('#"+settings["ID"]+"').dispatchEvent(new CustomEvent('wheel_'+d,{detail:e})); },{passive:false});");
	
}

void Applet::reload () {
	update();
}

void Applet::unload () {
	// TODO: clear event listeners
}


std::time_t Applet::update(){
	
	std::array<char, 1024> buffer;
	std::string output;

	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(settings["command"].c_str(), "r"), pclose);
	
	if (pipe) {
		
		while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
			output += buffer.data();
		}

		if (!output.empty() && output.back() == '\n') {  // output should be oneliner 
			output.pop_back();
		}
		
	}
	else {
		output = "err";
	}
	
	//window->html_element( "#"+settings["NAME"], "innerHTML", str_replace(settings["content"], "$VALUE", output) ); // HUI bug -- no, just missing ''
	window->call_js( "document.querySelector('#"+settings["ID"]+"').innerHTML = '"+ str_replace(settings["content"], "$VALUE", output)+"';" );
	
	
	return std::time(nullptr)+std::stoi(settings["update_interval"]);
}
