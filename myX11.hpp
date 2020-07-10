#pragma once

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <string>
#include <cassert>
#include <optional>
#include <vector>

extern Display* disp;

typedef std::vector<Window> WinVec;

/// Atoms
extern Atom wm_name;
extern Atom wm_desktop;
extern Atom net_current_desktop;
extern Atom net_wm_state;
extern Atom net_virtual_roots;
extern Atom net_active_window;
extern Atom net_desktop_geometry;
extern Atom state_modal;
extern Atom state_sticky;
extern Atom state_maximized_vert;
extern Atom state_maximized_horiz;
extern Atom state_shaded;
extern Atom state_skip_taskbar;
extern Atom state_skip_pager;
extern Atom state_hidden;
extern Atom state_fullscreen;
extern Atom state_above;
extern Atom state_below;
extern Atom state_demands_attention;

Atom find_atom (const char* desc);
void get_win_name (int win, char** buf, int sz);
void init_myX11 ();

template<int I, int N>
struct PropTraits;


template<int N> struct PropTraits<XA_ATOM,N> { typedef std::vector<Atom> type; };
template<> struct PropTraits<XA_ATOM,1> { typedef std::optional<Atom> type; };

template<> struct PropTraits<XA_STRING,1> { typedef std::optional<std::string> type; };

template<int N> struct PropTraits<XA_WINDOW,N> {  typedef std::vector<Window> type;  };
template<> struct PropTraits<XA_WINDOW,1> {  typedef std::optional<Window> type;  };

template<int N> struct PropTraits<XA_CARDINAL,N> {  typedef std::vector<unsigned int> type; };
template<> struct PropTraits<XA_CARDINAL,1> { typedef std::optional<unsigned int> type; };

template<int N, int C, typename T = typename PropTraits<C,N>::type>
struct XProp {
    static T get (Window win, Atom property)
    {
        Atom atype = 0;
        int fmt;
        unsigned long nitems = 0;
        unsigned long nbytes = 0;
        unsigned char* prop = nullptr;
        auto r = XGetWindowProperty(disp, win, property, 0, N, False, C, &atype, &fmt, &nitems, &nbytes, &prop);
        assert(r == Success);

        if (atype == 0) {
            return T();
        }

        assert (C == atype);

        typedef typename T::value_type V;

        if constexpr(N==1) {
            V result = *reinterpret_cast<V*>(prop);
            return T(result);
        } else {
            V* begin = reinterpret_cast<V*>(prop);
            V* end = begin + nitems;
            T result(begin,end);
            XFree(prop);
            return result;
        }
    
    }
};

template<>
struct XProp<1, XA_STRING>
{
    typedef typename PropTraits<XA_STRING,1>::type T;

    static T get (Window win, Atom property)
    {
        Atom atype = 0;
        int fmt;
        unsigned long nitems = 0;
        unsigned long nbytes = 0;
        unsigned char* prop = nullptr;
        auto r = XGetWindowProperty(disp, win, property, 0, 1024, False, XA_STRING, &atype, &fmt, &nitems, &nbytes, &prop);
        assert(r == Success);

        if (atype == 0) {
            return T();
        }
        
        //assert (XA_STRING == atype);

        if (nitems == 0) {
            return T();
        } else {
            const char *sz = reinterpret_cast<const char*> (prop);
            std::string s( sz );
            XFree(prop);
            return T(s);
        }
    
    }
};

extern void XClientSend (Atom atom, Window win, long x0, long x1, long x2, long x3, long x4);
extern void XClientSend (const char* property, Window win, long x0, long x1, long x2, long x3, long x4);