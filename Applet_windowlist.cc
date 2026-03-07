#define PANEL_APPLET
#include "./Applet.h"
#include "./utils/wlr_windowlist.cc"

WindowList list;


const std::map<std::string, void(*)(std::string)> Applet::commands () {
	return {};
}
const std::map<std::string,std::string> Applet::settings_default () {
	return { {"mode","icon state"}, {"icon_path","/usr/share/icons/hicolor/{scalable,48x48,32x32}/apps/"}, {"icon_default",""}, {"text_source","name"}, {"progress_whitelist",""}, {"progress_blacklist",""} };
}


void Applet::load() {
	list.init();
	window->html_element( "#"+settings["ID"], "innerHTML", "`<style>#"+settings["ID"]+"{display: flex;}  #"+settings["ID"]+" > div{color: white; margin: auto 3px; height: 100%; display: flex; align-items: center; background-size:50%; background-repeat:no-repeat; background-position:center; min-width:60px;}  #"+settings["ID"]+" > div:hover{color: red;}</style>`" );
	// TODO: vertical-compatible layout (use same code as panel)
	
	window->call_js("\
		['click', 'dragenter'].forEach(n => document.querySelector('#"+settings["ID"]+"').addEventListener(n, function(e) {\
		(" + window->call_native([](std::vector<std::string> argsv) -> void {
			
			
			void* ptr = reinterpret_cast<void*>(std::stoull(argsv[0]));
			if (!list.windowlist.contains(ptr)) return;
			
			auto& w = list.windowlist[ptr];
			
			if (argsv[1] == "click"){
				if (w.activated and !w.minimized) {
					zwlr_foreign_toplevel_handle_v1_set_minimized((zwlr_foreign_toplevel_handle_v1*)w.handle);
				}
				else {
					zwlr_foreign_toplevel_handle_v1_unset_minimized((zwlr_foreign_toplevel_handle_v1*)w.handle);  // doesnt work when the window is already unminimized but behind another window
					zwlr_foreign_toplevel_handle_v1_activate((zwlr_foreign_toplevel_handle_v1*)w.handle, /*global from header*/ seat);
				}
			}
			else if (argsv[1] == "dragenter"){
				zwlr_foreign_toplevel_handle_v1_unset_minimized((zwlr_foreign_toplevel_handle_v1*)w.handle);  // doesnt work when the window is already unminimized but behind another window
				zwlr_foreign_toplevel_handle_v1_activate((zwlr_foreign_toplevel_handle_v1*)w.handle, /*global from header*/ seat);
			}
			
			wl_display_roundtrip(list.display);
			
		}) + ") (e.target.closest('#"+settings["ID"]+" > div').id.substr(2), e.type) })); \
	");
}

void Applet::reload () {
}

void Applet::unload () {
	list.clear();
	// TODO
}

std::time_t Applet::update () {
	list.handle();
	
	for (auto& [k,w] : list.windowlist){
		if (w.created) {
			w.created = false;
			window->html_element( "#"+settings["ID"]+" div#w-"+std::to_string(reinterpret_cast<uintptr_t>(w.handle)) ); // TODO: does this work correctly? - todo fix invalid selector when > (HUI)
			
			if (settings["mode"].find("icon") != -1) {
				// TODO: apply lookup path, fallback icon (in case none set, construct custom svg from app_id)
				window->html_element( "#"+settings["ID"]+" div#w-"+std::to_string(reinterpret_cast<uintptr_t>(w.handle)), "style.backgroundImage", "`url(\""+get_xdg_icon(w.app_id,settings["icon_path"],settings["icon_default"])+"\")`");
			}
		}
		else if (w.changed) {
			w.changed = false;
			
			if (settings["mode"].find("text") != -1) {
				std::string text;
				
				if (settings["text_source"] == "name") {
					size_t pos = w.title.rfind(' ');
					if (pos != -1) text = w.title.substr(pos);
					else if (w.title.length() >= 2 /*and w.title.length() <= 20*/) text = w.title.substr(0,20);
					else text = "Untitled";
				}
				
				else if (settings["text_source"] == "title") {
					text = w.title;
				}
				
				else if (settings["text_source"] == "app_id") {
					text = w.app_id;
				}
				
				window->html_element( "#"+settings["ID"]+" > div#w-"+std::to_string(reinterpret_cast<uintptr_t>(w.handle)), "innerHTML", "`"+text+"`" ); // TODO: sanitize text
			}
			
			if (settings["mode"].find("state") != -1) {
				if (w.activated) window->html_element( "#"+settings["ID"]+" div#w-"+std::to_string(reinterpret_cast<uintptr_t>(w.handle)), "style.backgroundColor", "'rgba(255,255,255,0.15)'");
				else  window->html_element( "#"+settings["ID"]+" div#w-"+std::to_string(reinterpret_cast<uintptr_t>(w.handle)), "style.backgroundColor", "''");
			}
			
			if (settings["mode"].find("progress") != -1) {
				// TODO: parse progress from app titles
			}
			
		}
		else if (w.closed) {
			w.closed = false;
			window->html_element( "#"+settings["ID"]+" > div#w-"+std::to_string(reinterpret_cast<uintptr_t>(w.handle)), "remove()" ); // TODO: suboptimal cos attempts return
		}

	} 
	
	return std::time(nullptr)+1;
}