#include <dlfcn.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <ctime>
#include <set>
#include <vector>
#include <thread>
#include <utility>
#include <unistd.h>
#include "./Applet.h"
#include "HUI.hh"
#if defined(__linux__)
 #include <sys/inotify.h>
#endif

using namespace std::chrono_literals;
std::map<std::string, Applet*(*)()> plugins;  // constructors
std::map<std::string, Applet*> applets;  // applet instances
std::vector<std::pair<std::time_t, /*std::time_t(Applet::*)()*/Applet*>> updaters; // update functions


bool loader (std::map<std::string,std::string> settings) { 
	
	// load plugin if not already loaded
	if (!plugins.contains(settings["APPLET"])){
		
		std::string filename = std::string() + "/usr/lib/li_panel/" + "Applet_" + settings["APPLET"] + ".so" ; // TODO: use relative path
		void* handle = dlopen(filename.c_str(), RTLD_LAZY);
		if (!handle) return false;
		
		auto create = (Applet*(*)()) dlsym(handle, "create_object");
		//auto destroy = (void(*)(Applet*)) dlsym(handle, "destroy_object"); // TODO: clean deletion
		if (!create) return false;
	
		plugins[settings["APPLET"]] = create;
	
	}
	

	// create applet instance
	auto applet = plugins[settings["APPLET"]]();
	
	applet->settings = applet->settings_default();
	for (const auto& s : settings) applet->settings[s.first] = s.second;
	// TODO: check if ID and PLACEMENT valid
	
	
	// get webview
	applet->settings["PLACEMENT"] += " ";
	std::string parent = applet->settings["PLACEMENT"].substr(0,applet->settings["PLACEMENT"].find(' '));
	
	if (parent == "NULL") {
		applet->window = nullptr;
		applets.emplace(applet->settings["ID"], std::move(applet));
		return true;
	}
	else if (parent == "WINDOW") {
		applet->window = new HUI::WebView();
		applet->window->load_str(R"(<!DOCTYPE html>
<html>
  <head>
  <meta name="viewport" content="width=device-width, height=device-height, initial-scale=1.0"> 
    <style>
		html,body {
            height: 100vh;
            margin: 0;
            padding: 0;
            /*overflow: hidden;*/
		}
		body {
		  display: block;
		  /*flex-direction: row;*/
		  width: 100%;
		  text-align: center; /* works only when its in element style property for some reason */ 
		  background-color: rgba(0,0,0,0.5);
		}
		body > div {
			position: absolute;
			top: 0;
			bottom: 0;
			left: 0;
			right: 0;
		}
    </style>
  </head>
  <body id="body" class="container" style="text-align: center;"></body>
</html>)");
		// TODO: delete when removed
		parent = "body";
	}
	else if (applets.contains(parent)) {
		applet->window = applets[parent]->window;
	}
	else return false;
	
	
	// create div with id
	applet->window->call_js(std::string() + "\
		var template = document.createElement('template'); \
		template.innerHTML = `<div class=\"applet\" id=\"" + applet->settings["ID"] + "\" style=\"" + applet->settings["PLACEMENT"].substr(applet->settings["PLACEMENT"].find(' ')) + "\"></div>`; \
		document.querySelector('#" + parent + ".container, #" + parent + " .container').appendChild(template.content.firstChild); \
		");
	// TODO: check whether parent is container
	
	
	// done, call load()
	applet->settings["PLACEMENT"].pop_back();
	applet->load();
	applets.emplace(applet->settings["ID"], std::move(applet));
	// TODO: return result of load
	
	return true;
}

bool unloader (std::map<std::string,std::string> settings) {
	
	// unload content of container (if any) first
	if (applets[settings["ID"]]->window != nullptr) {
		for (auto& a : applets) {
			if ((a.second->settings["PLACEMENT"]+" ").starts_with(settings["ID"]+" ")) {
				unloader({{"ID",a.second->settings["ID"]}});
			}
		}
	}
	
	// propagate unload
	applets[settings["ID"]]->unload();
	
	// remove html
	if (applets[settings["ID"]]->window != nullptr) {
		// TODO: add 'applet' class to query  
		applets[settings["ID"]]->window->call_js("document.querySelector('#" + settings["ID"] + "').remove();");
	}
	
	// destroy window if it was created (for given applet)
	if (applets[settings["ID"]]->settings["PLACEMENT"].starts_with("WINDOW")) {
		delete applets[settings["ID"]]->window;
	}
	
	// TODO: unload plugins if no instance is left
	
	applets.erase(settings["ID"]);
	return true;
}

bool reloader (std::map<std::string,std::string> settings) {

	// we depend on applet ids
	if (!settings.contains("ID")) return false;

	// plugin doesnt exist (so load it)
	if (!applets.contains(settings["ID"])) return loader(settings);

	// create normalized new settings
	std::map<std::string,std::string> settings_new = plugins[settings["APPLET"]]()->settings_default();
	for (const auto& s : settings) settings_new[s.first] = s.second;

	// in case no settings have changed, return
	if (settings_new == applets[settings_new["ID"]]->settings) return true;

	// applet type has changed or has changed location (so unload+load)
	// TODO: move within same window with multiple containers
	if (applets[settings_new["ID"]]->settings["APPLET"] != settings_new["APPLET"] or applets[settings_new["ID"]]->settings["PLACEMENT"] != settings_new["PLACEMENT"]) {
		unloader({{"ID",settings_new["ID"]}});
		return loader(settings_new);
	}
	
	// perform regular update
	applets[settings_new["ID"]]->settings = settings_new;
	applets[settings_new["ID"]]->reload();
	applets[settings_new["ID"]]->update();
	return true;

	// TODO: reload .so plugin files
}


std::vector<std::map<std::string,std::string>> parser (std::string filename) {
	
	// open file
	std::ifstream file(filename);
    if (!file) {
        // TODO: handle error
        return {};
    }
	
	std::vector<std::map<std::string,std::string>> cfg;
	std::map<std::string,std::string> c;
	
	c["APPLET"] = "CORE";
	
	std::string line;
	while (std::getline(file, line)) {
	
		// TODO: trim (only) start - line, key, value
		if (line.starts_with("#")) {
			// comment
		}
		else if (line.find(" = ") != std::string::npos) {
			// key - value (ini)
			c[line.substr(0,line.find(" = "))] = line.substr(line.find(" = ")+3);
		}
		else if (line.find(": ") != std::string::npos) {
			// key - value (old)
			c[line.substr(0,line.find(": "))] = line.substr(line.find(": ")+2);
		}
		else if (line.find("{") != std::string::npos) {
			// next section (old)
			cfg.push_back(std::move(c));
			c.clear();
			
			c["APPLET"] = line.substr(0,line.find("{"));
		}
		else if (line.find("[") != std::string::npos and line.find("]") != std::string::npos) {
			// next section (ini)
			cfg.push_back(std::move(c));
			c.clear();
			
			c["APPLET"] = line.substr(line.find("[")+1,line.find("]")-(line.find("[")+1));
		}
		else {
			// TODO: handle error
		}
		
	}
	
	cfg.push_back(std::move(c));
	return cfg;
	
}

void configure(std::vector<std::map<std::string,std::string>>& cfg){
	// TODO: auto-set ID by order if not set
	
	// step 0: create list of applets for later deletion
	std::set<std::string> removed;
	for (auto& a : applets) {
		removed.insert(a.second->settings["ID"]);
	}
	for (auto& c : cfg) {
		removed.erase(c["ID"]);
	}
	
	// step 1: add new plugins
	for (auto c = cfg.begin(); c != cfg.end(); ) {
		if (!applets.contains((*c)["ID"])) {
			loader(*c);
			c = cfg.erase(c);
		}
		else ++c;
	}
	
	// step 2: update existing plugins if their config changed and/or relocate them
	for (auto& c : cfg) {
		reloader(c);
	}
	
	// step 3: remove what was removed
	for (auto& r : removed){
		unloader({{"ID",r}});
	}

}


int main (int argc, char* argv[]) {
	
	// load configuration
	std::string cfg_file;
	if (argc == 2) cfg_file = argv[1];
	else {
		for (auto f : {"$HOME/.config/li-panel/li-panel.cfg", "$HOME/.config/li-panel.cfg", "li-panel.cfg"}) {
			cfg_file = HUI::filepath(f).c_str();
			if (std::filesystem::exists(cfg_file)) break;
		}
	}
	std::cerr << "load config: " << cfg_file << "\n";
	auto cfg = parser(cfg_file);
	configure(cfg);
	
	// assign updaters
	// TODO: handle updaters in loader/unloader
	std::time_t t = std::time(nullptr);
	for (auto a : applets) updaters.push_back({t, a.second/*->update*/});
	
	// watch for cfg file changes
	#if defined(__linux__)
	int fd_inotify = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
	inotify_add_watch(fd_inotify, std::filesystem::path(cfg_file).parent_path().c_str(), IN_CLOSE_WRITE | IN_CREATE | IN_MOVED_TO);
	char buf_inotify[4096];
	#endif
	
	while (1) {
		
		// handle HUI
		HUI::WebView::handle_once();
		
		// get some sleep (constant value is not ideal, but should just work)
		std::this_thread::sleep_for(35ms);
		
		// handle applets
		t = std::time(nullptr);
		for (auto& u : updaters) if (u.first <= t) u.first = u.second->update();
		
		// keep up with cfg file changes
		#if defined(__linux__)
		if (read(fd_inotify, buf_inotify, sizeof(buf_inotify)) > 0) {
			std::cerr << "reload config: " << cfg_file << "\n";
			auto cfg = parser(cfg_file);
			configure(cfg);
			while (read(fd_inotify, buf_inotify, sizeof(buf_inotify)) > 0);
		}
		#endif
		
	}
	
}
