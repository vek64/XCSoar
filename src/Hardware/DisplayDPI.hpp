/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_HARDWARE_DISPLAY_DPI_HPP
#define XCSOAR_HARDWARE_DISPLAY_DPI_HPP

#include "Compiler.h"

#if defined(USE_FB) || defined(MESA_KMS) || defined(ANDROID)
#define HAVE_DPI_DETECTION
#endif

namespace Display {
  /**
   * Sets the displays x/y DPI
   * @param x Number of pixels per logical inch along the screen width
   * @param y Number of pixels per logical inch along the screen height
   */
  void SetDPI(unsigned x_dpi, unsigned y_dpi);

#ifdef HAVE_DPI_DETECTION
/**
 * This function gets called by our UI toolkit (the "Screen" library)
 * after it has determined the DPI value of the screen.
 */
void
ProvideDPI(unsigned x_dpi, unsigned y_dpi) noexcept;

/**
 * This function gets called by our UI toolkit (the "Screen" library)
 * after it has determined the physical dimensions of the screen.
 */
void
ProvideSizeMM(unsigned width_pixels, unsigned height_pixels,
             unsigned width_mm, unsigned height_mm) noexcept;
#endif

  /**
   * Returns the number of pixels per logical inch along the screen width
   * @return Number of pixels per logical inch along the screen width
   */
#ifndef __APPLE__
  gcc_const
#endif
  unsigned GetXDPI();
  /**
   * Returns the number of pixels per logical inch along the screen height
   * @return Number of pixels per logical inch along the screen height
   */
#ifndef __APPLE__
  gcc_const
#endif
  unsigned GetYDPI();
}

#endif
