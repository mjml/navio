#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/Xmu/WinUtil.h>
#include <stdio.h>
#include <algorithm>

#include <cassert>
#include <list>
using namespace std;

#include "myX11.hpp"
#include "navio.hpp"

void build_window_index ();
void examine_window (Window win);
void run_loop ();
void event_predicate ();

void handle_configure (XConfigureEvent& cev);
void handle_visibility (XVisibilityEvent& vev);
void handle_keypress (XKeyPressedEvent& kev);

void move (unsigned int timestamp, std::function<int(Point&, Point&)> scorefcn);
void move_left(unsigned int timestamp);
void move_right(unsigned int timestamp);
void move_up(unsigned int timestamp);
void move_down(unsigned int timestamp);

WindowGeom wingeo;


int main (int argc, char* argv[]) 
{
    init_myX11();

    build_window_index();

    //run_loop(); // runs forever
    move_left(0);

    while (1) {}

    XCloseDisplay(disp);

    return 0;
}

void build_window_index ()
{
    wingeo.clear();

    // Get the virtual desktop window (_NET_CURRENT_DESKTOP property of the root window)
    int rootwin = DefaultRootWindow(disp);
    Atom atype;
    int fmt = 0;
    unsigned long nitems = 0;
    unsigned long bytes = 0;
    unsigned char* prop = nullptr;

    // Find out how nicely our wm plays
    /*
    auto net_supported = XInternAtom(disp, "_NET_SUPPORTED", False);
    auto supatoms = XProp<1024,XA_ATOM>::get(rootwin,net_supported);
    for (auto atom : supatoms) {
        printf("WM supports %s\n", XGetAtomName(disp, atom));
    }
    */


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
        examine_window(topwin);
    }

    XFree(children);

}

void examine_window (Window win)
{
    Atom atype = 0;
    int fmt = 0;
    unsigned long nitems = 0;
    unsigned long nbytes = 0;
    unsigned char* prop = nullptr;

    bool valid = true;
    bool taskbar = true;

    int rootwin = DefaultRootWindow(disp);
    auto current_desktop = XProp<1,XA_CARDINAL>::get(rootwin, net_current_desktop);

    win = XmuClientWindow(disp, win);

    auto name = XProp<1,XA_STRING>::get(win,wm_name);
    auto desktop = XProp<1,XA_CARDINAL>::get(win,wm_desktop);

    fflush(stdout);

    if (nitems) {
        XFree(prop);
        prop = nullptr;
    }

    // only consider the windows on the taskbar
    Atom* aa = reinterpret_cast<Atom*>(prop);
    for (int i=0; i < nitems; i++) {
        valid &= !(aa[i] == state_skip_taskbar);
        valid &= !(aa[i] == state_hidden);
    }
    valid &= !current_desktop.has_value() || current_desktop == desktop;
    if (valid) {
        if (name.has_value()) {
            printf("[%lu, %s 0x%04x] ", win, name.value().c_str(), desktop.value_or(0xffff));
        } else {
            printf("[%lu - 0x%04x] ", win, desktop.value_or(0xffff));
        }
        
        Point loc;
        Window chld;
        XTranslateCoordinates(disp,win,rootwin,0,0,&loc.x,&loc.y,&chld);
        wingeo[win] = loc;
        printf("\n");
    } else {
        wingeo.extract(win);
    }
    
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

    win = XmuClientWindow(disp, win);
    examine_window(win);
    
}

void handle_visibility (XVisibilityEvent& vev)
{
    if (vev.state == VisibilityFullyObscured) {
        wingeo.extract(vev.window);
    } else {
        examine_window(vev.window);
    }
    
}

void handle_keypress (XKeyPressedEvent& kev)
{
    unsigned int timestamp = 0;
    if (kev.state & Mod4Mask) {
        if (kev.keycode == 38) { // a
            move_left(timestamp);
        } else if (kev.keycode == 40) { // d
            move_right(timestamp);
        } else if (kev.keycode == 39) { // s
            move_down(timestamp);
        } else if (kev.keycode == 25) { // w
            move_up(timestamp);
        }
    }

}

void move ( unsigned int ts, std::function<int(Point&, Point&)> scorefcn ) 
{
    int bestscore = 0x7fffffff;
    Window bestwin = 0;

    // get active window
    Window rootwin = DefaultRootWindow(disp);
    auto curwin = XProp<1,XA_WINDOW>::get(rootwin,net_active_window);
    Point src;
    if (curwin.has_value()) {
        Window chld;
        XTranslateCoordinates(disp,curwin.value(),rootwin,0,0,&src.x,&src.y,&chld);
    } else {
        auto vsz = XProp<2,XA_CARDINAL>::get(rootwin, net_desktop_geometry);
        src.x = vsz[0] / 2;
        src.y = vsz[1] / 2;
    }

    // cycle through wingeo looking for the closest distance
    for (auto it = wingeo.begin(); it != wingeo.end(); it++) {
        auto win = it->first;
        auto tgt = it->second;
        int score = scorefcn(src, tgt);
        printf("%lu score: %i\n", win, score);
        if (score >= 0 && score < bestscore && curwin != win) {
            bestscore = score;
            bestwin = win;
        }
    }
    printf("best score is %i, best window is %lu\n", bestscore, bestwin);

    auto desktop = XProp<1,XA_CARDINAL>::get(bestwin,XInternAtom(disp,"_NET_WM_DESKTOP",False));
    if (!desktop.has_value()) {
        desktop = XProp<1,XA_CARDINAL>::get(bestwin,XInternAtom(disp,"_WIN_WORKSPACE",False));
    }
    if (!desktop.has_value()) {
        printf("Warning: can't switch desktop.\n");
    } else {
        XClientSend(DefaultRootWindow(disp),XInternAtom(disp,"_NET_CURRENT_DESKTOP",False), desktop.value(), 0,0,0,0);
    }

    if (bestwin != 0) {
        printf("Switching to window 0x%08lx\n", bestwin);
        XClientSend("_NET_ACTIVE_WINDOW", bestwin, 0,0,0,0,0);
        XMapRaised(disp, bestwin);
    }

}


void move_left(unsigned int timestamp) {
    move( timestamp, std::function([](Point& src, Point& tgt) -> int { return (src.x - tgt.x) - abs(src.y - tgt.y); } ));
}

void move_right(unsigned int timestamp) {

}

void move_down(unsigned int timestamp) {

}

void move_up(unsigned int timestamp) {

}

void get_win_name (int win, char** buf, int sz);