#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/Xmu/WinUtil.h>
#include <stdio.h>

#include <cassert>
#include <list>
using namespace std;

#include "myX11.hpp"

void build_window_index ();
void examine_window (int win, std::optional<unsigned int> current_desktop);
void run_loop ();
void event_predicate ();
Atom find_atom (const char* desc);
void get_win_name (int win, char** buf, int sz);

void handle_configure (XConfigureEvent& cev);
void handle_visibility (XVisibilityEvent& vev);
void handle_keypress (XKeyPressedEvent& kev);

/// GLOBALS ///////
Display* disp = nullptr;

/// Atoms
Atom wm_name;
Atom net_current_desktop;
Atom net_wm_state;
Atom net_virtual_roots;
Atom state_modal;
Atom state_sticky;
Atom state_maximized_vert;
Atom state_maximized_horiz;
Atom state_shaded;
Atom state_skip_taskbar;
Atom state_skip_pager;
Atom state_hidden;
Atom state_fullscreen;
Atom state_above;
Atom state_below;
Atom state_demands_attention;

int main (int argc, char* argv[]) 
{
    disp = XOpenDisplay(nullptr);
    assert(disp != nullptr);

    // Set up the X11 environment and event masks
    int rootwin = DefaultRootWindow(disp);
    //int r = XSelectInput(disp, rootwin, KeyPress | VisibilityNotify);

    // Determine the atoms we need
    wm_name                 = find_atom("WM_NAME");
    net_current_desktop     = find_atom("_NET_CURRENT_DESKTOP");
    net_virtual_roots       = find_atom("_NET_VIRTUAL_ROOTS");
    net_wm_state            = find_atom("_NET_WM_STATE");    
    state_modal             = find_atom("_NET_WM_STATE_MODAL");
    state_sticky            = find_atom("_NET_WM_STATE_STICKY");
    state_maximized_vert    = find_atom("_NET_WM_STATE_MAXIMIZED_VERT");
    state_maximized_horiz   = find_atom("_NET_WM_STATE_MAXIMIZED_HORZ");
    state_shaded            = find_atom("_NET_WM_STATE_SHADED");
    state_skip_taskbar      = find_atom("_NET_WM_STATE_SKIP_TASKBAR");
    state_skip_pager        = find_atom("_NET_WM_STATE_SKIP_PAGER");
    state_hidden            = find_atom("_NET_WM_STATE_HIDDEN");
    state_fullscreen        = find_atom("_NET_WM_STATE_FULLSCREEN");
    state_above             = find_atom("_NET_WM_STATE_ABOVE");
    state_below             = find_atom("_NET_WM_STATE_BELOW");
    state_demands_attention = find_atom("_NET_WM_STATE_DEMANDS_ATTENTION");

    build_window_index();

    run_loop(); // runs forever

    return 0;
}

void build_window_index ()
{
    // Get the virtual desktop window (_NET_CURRENT_DESKTOP property of the root window)
    int rootwin = DefaultRootWindow(disp);
    Atom atype;
    int fmt = 0;
    unsigned long nitems = 0;
    unsigned long bytes = 0;
    unsigned char* prop = nullptr;

    // get the current desktop
    auto desktop = XProp<1,XA_CARDINAL>::get(rootwin, net_current_desktop);
    if (desktop.has_value()) {
        printf("Currently on desktop %u\n", desktop.value());
    } else {
        printf("Not a desktop wtf!!!\n");
    }

    // get the children
    Window rootwin2;
    Window dparent;
    Window* children;
    unsigned int nchildren = 0;
    Status sr = XQueryTree(disp, rootwin, &rootwin2, &dparent, &children, &nchildren);
    assert(sr != 0);
    assert(rootwin == rootwin2);
    assert(nchildren > 0);
    printf("root children: %d\n", nchildren);

    // get the children via _NET_CLIENT_LIST

    // for each window below the root, check if it's a desktop window
    for (int c=0; c < nchildren; c++) {
        Window topwin;
        topwin = children[c];
        examine_window(topwin, desktop);
    }

    XFree(children);

}

void examine_window (int win, std::optional<unsigned int> current_desktop)
{
    Atom atype = 0;
    int fmt = 0;
    unsigned long nitems = 0;
    unsigned long nbytes = 0;
    unsigned char* prop = nullptr;

    bool valid = true;
    bool taskbar = true;

    auto r = XGetWindowProperty(disp, win, wm_name, 0, 1024, False, XA_STRING, &atype, &fmt, &nitems, &nbytes, &prop);
    assert(r==Success);
    if(fmt == 8 && atype == XA_STRING) {
        printf("[%s] ", prop);
        fflush(stdout);
    } else {
        printf("[] ");
    }

    auto name = XProp<1,XA_STRING>::get(win,wm_name);

    if (nitems) {
        XFree(prop);
        prop = nullptr;
    }

    Window client = XmuClientWindow(disp, win);

    r = XGetWindowProperty(disp, win, net_wm_state, 0, 128, False, XA_ATOM, &atype, &fmt, &nitems, &nbytes, &prop);
    assert(r == Success);
    if (atype == 0) {
        printf("\n");
        return;
    }
    assert(atype == XA_ATOM);
    assert(fmt == 32);

    // only consider the windows on the taskbar
    Atom* aa = reinterpret_cast<Atom*>(prop);
    for (int i=0; i < nitems; i++) {
        valid &= !(aa[i] == state_skip_taskbar);
        valid &= !(aa[i] == state_hidden);
    }
    printf("%s\n", valid ? "task" : "hidden/utility");
    
    if (nitems) {
        XFree(prop);
        prop = nullptr;
    }

}


void run_loop ()
{
    // respond to events:
    XEvent event;

    while (1) {

        // window focus / selection
        XNextEvent(disp, &event);

        // window move / resize
        if (event.type & ConfigureNotify) {
            XConfigureEvent cev = event.xconfigure;
            handle_configure(cev);
        }

        // window visibility
        if (event.type & VisibilityNotify) {
            XVisibilityEvent vev = event.xvisibility;
            handle_visibility(vev);
        }

        // key press (window switch commands)
        if (event.type & KeyPress) {    
            XKeyPressedEvent kev = event.xkey;
            handle_keypress(kev);
        }

    }

}

void handle_configure (XConfigureEvent& cev)
{
    Window win = cev.window;
    Window parent;
    Window root;
    Window stateWin;
    Window* children = nullptr;
    unsigned int nchildren;
    
    int r = XQueryTree(disp, win, &root, &parent, &children, &nchildren);
    assert(r);
    if (children) {
        XFree(children);
    }

    stateWin = XmuClientWindow(disp, win);

    

    
}

void handle_visibility (XVisibilityEvent& vev)
{

}

void handle_keypress (XKeyPressedEvent& kev)
{

}

Atom find_atom (const char* desc) 
{
    auto r = XInternAtom(disp, desc, False);
    assert(r != None);
    return r;
}

void get_win_name (int win, char** buf, int sz);