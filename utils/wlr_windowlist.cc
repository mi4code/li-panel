#include <string>
#include <map>

#include <cstdio>
#include <cstring>

#include <glob.h>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <wayland-client.h>
#include "wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"
#include "wlr-foreign-toplevel-management-unstable-v1-client-protocol.c"

#define WLR_FOREIGN_TOPLEVEL_MANAGEMENT_VERSION 3

std::string file_to_data_url(const std::filesystem::path& path) {
    // --- read file ---
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Failed to open file");

    std::vector<unsigned char> data(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    // --- base64 encode ---
    static constexpr char table[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string encoded;
    encoded.reserve((data.size() + 2) / 3 * 4);

    int val = 0;
    int valb = -6;

    for (unsigned char c : data) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            encoded.push_back(table[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    if (valb > -6)
        encoded.push_back(table[((val << 8) >> (valb + 8)) & 0x3F]);

    while (encoded.size() % 4)
        encoded.push_back('=');

    // --- detect mime ---
    std::string mime = "application/octet-stream";
    auto ext = path.extension().string();

    if (ext == ".png")
        mime = "image/png";
    else if (ext == ".svg")
        mime = "image/svg+xml";
    else if (ext == ".xpm")
        mime = "image/x-xpixmap";
    else if (ext == ".jpg" || ext == ".jpeg")
        mime = "image/jpeg";
    else if (ext == ".gif")
        mime = "image/gif";

    return "data:" + mime + ";base64," + encoded;
}

std::string get_xdg_icon(const std::string& app_id, const std::string& icon_path = "/usr/share/icons/hicolor/{scalable,48x48,*}/apps/", const std::string& icon_default = ""){
    glob_t g;
    std::string pattern = icon_path + app_id + ".*";

    if (glob(pattern.c_str(), GLOB_BRACE, nullptr, &g) == 0) {
        for (size_t i = 0; i < g.gl_pathc; ++i) {
            std::cerr << g.gl_pathv[i] << "\n";
        }
    }
	
	if (g.gl_pathc >= 1){
		std::string path = g.gl_pathv[0];
		globfree(&g);
		return file_to_data_url(path);
	}

    globfree(&g);
	if (icon_default != "") return file_to_data_url(icon_default);
	return "data:image/svg+xml;utf8,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 20'><text x='50' y='50%' text-anchor='middle' dominant-baseline='middle' fill='goldenrod'>" + app_id + "</text></svg>";
}

class WindowList {

	struct Window {
		void* handle = nullptr;

		// TODO: all values
		std::string title;
		std::string app_id;
		//int display; // aka wl output, this is to be reworked in the future because wayland allows windows to be present on multiple windows
		bool activated = false;
		bool minimized = false;
		/*bool maximized = false;
		bool fullscreen = false;*/
		// uint8t progress; // parsed from title if not supported
		
		char created = false;
		char changed = false;
		char closed = false;
		// TODO: (one bool-compatible value, !=0 means something changed, value defines what did); any change, title, id, state, output, parent, closed
		
		// TODO: make this part of HUI::WindowControls (close/toggle/minimize/show/maximize/unmaximize, title, app_id, icon, progress) and HUI::App (name, icon, command) 
	};

  public:
	WindowList(){};
	~WindowList(){};

	bool init ();
	bool handle ();
	bool clear ();

	std::map<void*, Window> windowlist;

	/*struct pImpl;
	std::unique_ptr<pImpl> impl; */

	wl_display* display;
	wl_registry* registry;
};

#if defined(WINDOWLIST_TEST) 
	#define printdbg(...) fprintf(stderr, __VA_ARGS__)
#else
	#define printdbg(...) 
#endif

static const struct zwlr_foreign_toplevel_handle_v1_listener toplevel_impl = {
	.title =  [](void* data, struct zwlr_foreign_toplevel_handle_v1* zwlr_toplevel, const char* title) -> void {
		printdbg("WINDOW: %p TITLE: %s\n",(void*)zwlr_toplevel, title);
		
		static_cast<WindowList*>(data)->windowlist[zwlr_toplevel].title = title;
	},
	.app_id = [](void* data, struct zwlr_foreign_toplevel_handle_v1* zwlr_toplevel, const char* app_id) -> void {
		printdbg("WINDOW: %p APP_ID: %s\n",(void*)zwlr_toplevel, app_id);
		
		static_cast<WindowList*>(data)->windowlist[zwlr_toplevel].app_id = app_id;
	},
	.output_enter = [](void *data, struct zwlr_foreign_toplevel_handle_v1 *zwlr_toplevel, struct wl_output *output) -> void {
		printdbg("WINDOW: %p ENTERED OUTPUT: %u\n",(void*)zwlr_toplevel, (uint32_t)(size_t)wl_output_get_user_data(output)); // never runs
	},
	.output_leave = [](void *data, struct zwlr_foreign_toplevel_handle_v1 *zwlr_toplevel, struct wl_output *output) -> void {
		printdbg("WINDOW: %p LEFT OUTPUT: %u\n",(void*)zwlr_toplevel, (uint32_t)(size_t)wl_output_get_user_data(output)); // never runs
	},
	.state =  [](void *data, struct zwlr_foreign_toplevel_handle_v1 *zwlr_toplevel, struct wl_array *array) -> void {
		uint8_t state2 = 0;
		for (uint32_t* pos = (uint32_t*) (array)->data; (const char *) pos < ((const char *) (array)->data + (array)->size); (pos)++){
			if (*pos == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MAXIMIZED)  state2 |= (1 << 0);
			if (*pos == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED)  state2 |= (1 << 1);
			if (*pos == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED)  state2 |= (1 << 2);
			if (*pos == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_FULLSCREEN) state2 |= (1 << 3);
		}
		printdbg("WINDOW: %p STATE: %c%c%c%c\n",(void*)zwlr_toplevel,  ((state2) & 0x08 ? '1' : '0'),((state2) & 0x04 ? '1' : '0'),((state2) & 0x02 ? '1' : '0'),((state2) & 0x01 ? '1' : '0')  );
		
		static_cast<WindowList*>(data)->windowlist[zwlr_toplevel].activated = false;
		static_cast<WindowList*>(data)->windowlist[zwlr_toplevel].minimized = false;
		for (uint32_t* pos = (uint32_t*) (array)->data; (const char *) pos < ((const char *) (array)->data + (array)->size); (pos)++){
			if (*pos == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED)  static_cast<WindowList*>(data)->windowlist[zwlr_toplevel].activated = true;
			if (*pos == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_MINIMIZED)  static_cast<WindowList*>(data)->windowlist[zwlr_toplevel].minimized = true;
		}
	},
	.done =   [](void* data, struct zwlr_foreign_toplevel_handle_v1* zwlr_toplevel) -> void {
		printdbg("WINDOW: %p DONE\n",(void*)zwlr_toplevel); // hapens before close

		static_cast<WindowList*>(data)->windowlist[zwlr_toplevel].changed = true;
	},
	.closed = [](void* data, struct zwlr_foreign_toplevel_handle_v1* zwlr_toplevel) -> void {
		printdbg("WINDOW: %p CLOSED\n",(void*)zwlr_toplevel);

		static_cast<WindowList*>(data)->windowlist[zwlr_toplevel].closed = true;
		
		zwlr_foreign_toplevel_handle_v1_destroy(zwlr_toplevel);
	},
	.parent = [](void *data, struct zwlr_foreign_toplevel_handle_v1 *zwlr_toplevel, struct zwlr_foreign_toplevel_handle_v1 *zwlr_parent) -> void {
		printdbg("WINDOW: %p PARENT: %p\n",(void*)zwlr_toplevel, (void*)zwlr_parent);
	}
};

static const struct zwlr_foreign_toplevel_manager_v1_listener toplevel_manager_impl = {
	.toplevel = [](void *data, struct zwlr_foreign_toplevel_manager_v1 *toplevel_manager, struct zwlr_foreign_toplevel_handle_v1 *zwlr_toplevel) -> void { 
		printdbg("WINDOW: %p NEW\n",(void*)zwlr_toplevel); // runs every time new toplevel created (seen on startup)

		zwlr_foreign_toplevel_handle_v1_add_listener(zwlr_toplevel, &toplevel_impl, /*WindowList*/data); // set up callbacks for window changes

		static_cast<WindowList*>(data)->windowlist[zwlr_toplevel].created = true;
		static_cast<WindowList*>(data)->windowlist[zwlr_toplevel].handle = zwlr_toplevel;
	},
	.finished = [](void *data, struct zwlr_foreign_toplevel_manager_v1 *toplevel_manager) -> void {
		zwlr_foreign_toplevel_manager_v1_destroy(toplevel_manager);
	}
};

static uint32_t pref_output_id = UINT32_MAX; // values 0/1 do nothing
static struct wl_output *pref_output = NULL;
static struct zwlr_foreign_toplevel_manager_v1 *toplevel_manager = NULL;
struct wl_seat *seat = NULL;

static const struct wl_registry_listener registry_listener = {
	.global = [](void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) -> void {
		if (strcmp(interface, wl_output_interface.name) == 0) {
			printdbg("wl_output_interface.name .. ");
			if (name == pref_output_id) {
				pref_output = (wl_output*) wl_registry_bind(registry, name, &wl_output_interface, version);
				printdbg("pref_output*");
			}
			printdbg("\n");
		} 
		else if (strcmp(interface,zwlr_foreign_toplevel_manager_v1_interface.name) == 0) {
			toplevel_manager = (zwlr_foreign_toplevel_manager_v1*) wl_registry_bind(registry, name, &zwlr_foreign_toplevel_manager_v1_interface, WLR_FOREIGN_TOPLEVEL_MANAGEMENT_VERSION);
			zwlr_foreign_toplevel_manager_v1_add_listener(toplevel_manager, &toplevel_manager_impl, /*WindowList*/data);
			printdbg("zwlr_foreign_toplevel_manager_v1_interface.name .. toplevel_manager*\n");
		} 
		else if (strcmp(interface, wl_seat_interface.name) == 0 && seat == NULL) {
			seat = (wl_seat*) wl_registry_bind(registry, name, &wl_seat_interface, version);
			printdbg("wl_seat_interface.name .. seat*\n");
		}
	},
	.global_remove = [](void *data, struct wl_registry *registry, uint32_t name) -> void {
		// TODO: cleanup
	},
};


#if defined(WINDOWLIST_TEST) 
int main(){
	auto wl = WindowList();
	wl.init();
	while(wl.handle());
#endif


bool WindowList::init () {
	display = wl_display_connect(NULL);
	if (display == NULL) {
		printdbg("Failed to create display\n");
		return false;
	}

	registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, (void*)this);
	wl_display_roundtrip(display);

	if (toplevel_manager == NULL) {
		printdbg("wlr-foreign-toplevel-management not available\n");
		return false;
	}
	
	wl_display_flush(display);
	return true;
}

bool WindowList::handle () {
	//return wl_display_dispatch(display) != -1; // blocks until next event
	wl_display_roundtrip(display); // doesnt block
	return true;
	// TODO: proper handle compatible with poll instead of delay-based handling
	// TODO: allow reconnect after compositor restart
}

bool WindowList::clear () {
	// TODO: free objects
}


/*struct WindowList::pImpl {

};*/
