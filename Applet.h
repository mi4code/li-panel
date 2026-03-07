#include "HUI.hh"
#include <string>
#include <map>
#include <ctime>

class Applet {
  public:
	Applet () {};
	
	virtual void load (); 
	virtual void unload ();
	virtual void reload ();
	virtual std::time_t update ();
	
	HUI::WebView* window = nullptr;
	std::map<std::string,std::string> settings;
	
	virtual const std::map<std::string, void(*)(std::string)> commands ();
	virtual const std::map<std::string,std::string> settings_default ();
};

#if defined(PANEL_APPLET)

extern "C" Applet* create_object () {
	return new Applet;
}

extern "C" void destroy_object (Applet* object) {
	delete object;
}

#endif
 