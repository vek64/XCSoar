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

#include "DebugPort.hpp"
#include "OS/Args.hpp"
#include "Device/Port/Port.hpp"
#include "Device/Port/ConfiguredPort.hpp"
#include "Device/Config.hpp"
#include "Operation/ConsoleOperationEnvironment.hpp"
#include "IO/Async/GlobalIOThread.hpp"
#include "Util/StaticString.hxx"
#include "Math/fixed.hpp"
#include "Time/PeriodClock.hpp"

#include <stdio.h>
#include <stdlib.h>

#ifdef __clang__
/* true, the nullptr cast below is a bad kludge */
#pragma GCC diagnostic ignored "-Wnull-dereference"
#endif

int main(int argc, char **argv)
{
  Args args(argc, argv, "PORT");
  const DeviceConfig config = ParsePortArgs(args);
  args.ExpectEnd();

  InitialiseIOThread();

  Port *port = OpenPort(config, nullptr, *(DataHandler *)nullptr);
  if (port == nullptr) {
    DeinitialiseIOThread();
    fprintf(stderr, "Failed to open port\n");
    return EXIT_FAILURE;
  }

  ConsoleOperationEnvironment env;

  if (!port->WaitConnected(env)) {
    delete port;
    DeinitialiseIOThread();
    fprintf(stderr, "Failed to connect the port\n");
    return EXIT_FAILURE;
  }

  PeriodClock start_clock;
  start_clock.Update();

  PeriodClock pressure_clock;
  PeriodClock battery_clock;

  fixed pressure = fixed(101300);
  unsigned battery_level = 11;
  while (true) {
    if (pressure_clock.CheckUpdate(48)) {
      NarrowString<16> sentence;

      int elapsed_ms = start_clock.Elapsed();
      fixed elapsed = fixed(elapsed_ms) / 1000;
      fixed vario = sin(elapsed / 3) * cos(elapsed / 10) *
        cos(elapsed / 20 + fixed(2)) * 3;

      fixed pressure_vario = -vario * fixed(12.5);
      fixed delta_pressure = pressure_vario * 48 / 1000;
      pressure += delta_pressure;

      sentence = "_PRS ";
      sentence.AppendFormat("%08X", uround(pressure));
      sentence += "\n";

      port->Write(sentence.c_str(), sentence.length());
    }

    if (battery_clock.CheckUpdate(11000)) {
      NarrowString<16> sentence;

      sentence = "_BAT ";
      if (battery_level <= 10)
        sentence.AppendFormat("%X", battery_level);
      else
        sentence += "*";

      sentence += "\n";
      port->Write(sentence.c_str(), sentence.length());

      if (battery_level == 0)
        battery_level = 11;
      else
        battery_level--;
    }
  }
}
