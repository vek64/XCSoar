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

#include "Angle.hpp"
#include <assert.h>

void
Angle::ToDMS(unsigned &dd, unsigned &mm, unsigned &ss, bool &is_positive) const
{
  is_positive = !negative(value);

  unsigned value = uround(AbsoluteDegrees() * 3600);

  ss = value % 60;
  value /= 60;

  mm = value % 60;
  value /= 60;

  dd = value;
}

void
Angle::ToDMM(unsigned &dd, unsigned &mm, unsigned &mmm,
             bool &is_positive) const
{
  is_positive = !negative(value);

  unsigned value = (unsigned) (AbsoluteDegrees() * 60000);
  dd = value / 60000;
  value %= 60000;
  mm = value / 1000;
  mmm = value % 1000;
}

int
Angle::Sign(const fixed tolerance) const
{
  if ((value > tolerance))
    return 1;
  if ((value < -tolerance))
    return -1;

  return 0;
}

int
Angle::Sign() const
{
  if (positive(value))
    return 1;
  if (negative(value))
    return -1;

  return 0;
}

fixed
Angle::AbsoluteDegrees() const 
{
  return Absolute().Degrees();
}

fixed
Angle::AbsoluteRadians() const 
{
  return Absolute().Radians();
}

Angle
Angle::AsBearing() const
{
  assert(!isnan(value));
  assert(!isinf(value));
  assert(fabs(value) < fixed(100) * FullCircle().Native());

#ifndef FIXED_MATH
  /* Workaround for endless loops below; this is only for release
     builds.  Debug builds will crash due to assertion failure.  Right
     now, I don't know where those abnormal values come from, but I
     hope we'll find out soon - until that happens, this workaround
     reduces some user frustration with XCSoar freezes. */
  if (!isnormal(value))
    return Zero();
#endif

  Angle retval(value);

  while (retval < Zero())
    retval += FullCircle();

  while (retval >= FullCircle())
    retval -= FullCircle();

  return retval;
}

Angle
Angle::AsDelta() const
{
  assert(!isnan(value));
  assert(!isinf(value));
  assert(fabs(value) < fixed(100) * FullCircle().Native());

#ifndef FIXED_MATH
  /* same workaround as in AsBearing() */
  if (!isnormal(value))
    return Zero();
#endif

  Angle retval(value);

  while (retval <= -HalfCircle())
    retval += FullCircle();

  while (retval > HalfCircle())
    retval -= FullCircle();

  return retval;
}

Angle
Angle::Reciprocal() const
{
  return (*this + HalfCircle()).AsBearing();
}

Angle
Angle::HalfAngle(const Angle end) const
{
  if (value == end.value) {
    return Reciprocal();
  } else if (value > end.value) {
    if ((*this - end) < HalfCircle())
      return (*this + end).Half().Reciprocal();
    else
      return (*this + end).Half();
  } else {
    if ((end - *this) < HalfCircle())
      return (*this + end).Half().Reciprocal();
    else
      return (*this + end).Half();
  }
}

Angle
Angle::Fraction(const Angle end, const fixed fraction) const
{
  if (value == end.value)
    return Angle(value);

  const Angle diff = Angle(end.value - value).AsDelta();
  return Angle(value + diff.value * fraction);
}

gcc_pure
bool
Angle::Between(const Angle start, const Angle end) const
{
  Angle width = (end - start).AsBearing();
  Angle delta = (*this - start).AsBearing();

  return delta <= width;
}

bool
Angle::CompareRoughly(Angle other, Angle threshold) const
{
  const Angle delta = (*this - other).AsDelta();
  return delta >= -threshold && delta <= threshold;
}
