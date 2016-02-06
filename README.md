# fpath-bezier

**NOTE** - This project is deprecated.  See [pebble-fctx](https://github.com/jrmobley/pebble-fctx) for
the latest.

This is a fork of [pebble-hacks/gpath-bezier](https://github.com/pebble-hacks/gpath-bezier).
It adds support for subpixel accurate rendering of filled paths.  Additionally, on color
Pebble devices, antialiasing is supported.

The UP button now switches between the native GPath rendering and the new FPath rendering
for comparison.  Also, the rotation rate has been slowed considerably to make it easier to
see just how effective the subpixel accuracy is at smoothing both the shape and the animation.

Written against the PebbleSDK v3.0-beta10

The interesting parts are derived from the following excellent resources:

*Perspective Texture Mapping*
Game Developer Magazine
Chris Hecker
Copyright (c) 1995-1997 Chris Hecker. All rights reserved.
[http://chrishecker.com/Miscellaneous_Technical_Articles](http://chrishecker.com/Miscellaneous_Technical_Articles)

*Scanline edge-flag algorithm for antialiasing*
EG UK Theory and Practice of Computer Graphics (2007)
Kiia Kallio
Copyright (c) 2005-2007 Kiia Kallio <kkallio@uiah.fi>
[http://mlab.uiah.fi/~kkallio/antialiasing/](http://mlab.uiah.fi/~kkallio/antialiasing/)

*The Edge Flag Algorithm--A Fill Method for Raster Scan Displays*
IEEE Transactions On Computers, Volume C-30, No. 1, January 1981.
Bryan D. Ackland and Neil H. Weste

The original README for pebble-hacks/gpath-bezier follows:

# gpath-bezier

Drawing complex paths requires a lot of manual work on Pebble. You can do this
efficiently and quickly using a Pebble-optimized ``GPath`` algorithm and Bezier
curves.

This library introduces the `GPathBuilder` API that allows you to
programmatically contruct `GPath` objects by specifying start and end points for
a curve. The builder than adds intermetiary points to make the curve as smooth
as is practical.

## Code Example

![screenshot](screenshots/screenshot1.png)

You can draw the path shown above using only the `GPathBuilder` code below:

    // Create GPathBuilder object
    GPathBuilder *builder = gpath_builder_create(MAX_POINTS);

    // Move to the starting point of the GPath
    gpath_builder_move_to_point(builder, GPoint(0, -60));
    // Create curve
    gpath_builder_curve_to_point(builder, GPoint(60, 0), GPoint(35, -60), GPoint(60, -35));
    // Create straight line
    gpath_builder_line_to_point(builder, GPoint(-60, 0));
    // Create another curve
    gpath_builder_curve_to_point(builder, GPoint(0, 60), GPoint(-60, 35), GPoint(-35, 60));
    // Create another straight line
    gpath_builder_line_to_point(builder, GPoint(0, -60));

    // Create GPath object out of our GPathBuilder object
    s_path = gpath_builder_create_path(builder);
    // Destroy GPathBuilder object
    gpath_builder_destroy(builder);

    // Get window bounds
    GRect bounds = layer_get_bounds(window_get_root_layer(window));
    // Move newly created GPath to the center of the screen
    gpath_move_to(s_path, GPoint((int16_t)(bounds.size.w/2), (int16_t)(bounds.size.h/2)));

## More Information

You can learn more about Bezier curves by reading the Pebble Developers blog 
post 
[*Bezier Curves and GPaths on Pebble*](https://developer.getpebble.com//blog/2015/02/13/Bezier-Curves-And-GPaths/).
