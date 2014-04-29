= HEALPix Viewer

Render a HEALPix data set in false color using modern OpenGL techniques. The
purpose of this project is mostly to bring myself up to speed with modern
OpenGL (and un-learn archaisms like `glBegin(GL_TRIANGLE_STRIP)`).

This is inspired by Damien George's excellent CMB visualization at
<http://thecmb.org>. However, it suffers from less distortion because it
implements the HEALPix projection itself (instead of a geodesic approximation
of it).

== Requirements

Building requires:

* GLFW <http://www.glfw.org>
* GLM <http://glm.g-truc.net>
* CHealpix <http://healpix.sourceforge.net>
* CFitsio <http://heasarc.gsfc.nasa.gov/fitsio/fitsio.html>
* a C++ compiler, `make`, and `pkgconfig`

If, like me, you are on a Mac with MacPorts (https://trac.macports.org), you
can get all of the dependencies with:

    sudo port install glfw glm healpix-c

== To build

Just run `make`.

== Usage

Just pass the name of a (possibly gzip-compressed) FITS file, like this:

    healpixviewer INPUT.fits[.gz]
