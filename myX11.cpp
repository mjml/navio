#include "myX11.hpp"


/// Atoms
Atom wm_name;
Atom wm_desktop;
Atom net_current_desktop;
Atom net_wm_state;
Atom net_virtual_roots;
Atom net_active_window;
Atom net_desktop_geometry;
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

/// GLOBALS ///////
Display* disp = nullptr;



void init_myX11 () {
        
    disp = XOpenDisplay(nullptr);
    assert(disp != nullptr);

    // Determine the atoms we need
    wm_name                 = find_atom("WM_NAME");
    wm_desktop              = find_atom("_NET_WM_DESKTOP");
    net_current_desktop     = find_atom("_NET_CURRENT_DESKTOP");
    net_virtual_roots       = find_atom("_NET_VIRTUAL_ROOTS");
    net_wm_state            = find_atom("_NET_WM_STATE");    
    net_active_window       = find_atom("_NET_ACTIVE_WINDOW");
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

}

Atom find_atom (const char* desc) 
{
    auto r = XInternAtom(disp, desc, False);
    assert(r != None);
    return r;
}

void XClientSend (Atom atom, Window win, long x0, long x1, long x2, long x3, long x4) 
{
    XEvent ev;
    ev.xclient.type = ClientMessage;
    ev.xclient.serial = 0;
    ev.xclient.send_event = True;
    ev.xclient.message_type = atom;
    ev.xclient.display = disp;
    ev.xclient.format = 32;
    ev.xclient.window = win;
    ev.xclient.data.l[0] = x0;
    ev.xclient.data.l[1] = x1;
    ev.xclient.data.l[2] = x2;
    ev.xclient.data.l[3] = x3;
    ev.xclient.data.l[4] = x4;
    if (XSendEvent(disp, DefaultRootWindow(disp), False, SubstructureRedirectMask | SubstructureNotifyMask, &ev)) {
        XFlush(disp);
        return;
    } else {
        throw std::runtime_error("Couldn't send X11 client message to root window.");
    }
}

void XClientSend (const char* property, Window win, long x0, long x1, long x2, long x3, long x4) 
{
    XClientSend(XInternAtom(disp, property, False), win, x0, x1, x2, x3, x4);
}