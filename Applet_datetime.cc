#define PANEL_APPLET
#include "./Applet.h"
#include <ctime>
#include <regex>


const std::map<std::string, void(*)(std::string)> Applet::commands () {
	return {};
}

const std::map<std::string,std::string> Applet::settings_default () {
	return { {"format","%H:%M"}, };
}


void Applet::load () {

	// TODO: dont use settings for _format value
	settings["_format"] = std::regex_replace(std::regex_replace(settings["format"],std::regex("(\\n)"),"\n"),std::regex("(\\t)"),"\t");
	
	window->html_element("#" + settings["ID"], "innerHTML", "'datetime error'");
	window->html_element("#" + settings["ID"], "style.display", "'flex'");
	window->html_element("#" + settings["ID"], "style.alignItems", "'center'");
	window->html_element("#" + settings["ID"], "style.justifyContent", "'center'");

}

void Applet::reload () {
	load ();
	update ();
}
void Applet::unload () {
	
}


std::time_t Applet::update () {
	std::time_t now = std::time(nullptr);
    std::tm* tm = std::localtime(&now);  // pointer to object in now, no need to free it
	char buffer[255];
    std::strftime(buffer, sizeof(buffer), settings["_format"].c_str(), tm);
	
	window->html_element("#" + settings["ID"], "innerHTML", std::string() + "'" + buffer + "'");
	return std::time(nullptr)+1;  // TODO: automatic optimal update interval
}