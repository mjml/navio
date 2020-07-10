#pragma once

#include <X11/Xlib.h>
#include <map>

struct Point
{
    int x;
    int y;
};

typedef std::map<Window,Point> WindowGeom;