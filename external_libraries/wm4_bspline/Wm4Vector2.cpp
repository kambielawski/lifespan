// Wild Magic Source Code
// David Eberly
// http://www.geometrictools.com
// Copyright (c) 1998-2008
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or (at
// your option) any later version.  The license is available for reading at
// either of the locations:
//     http://www.gnu.org/copyleft/lgpl.html
//     http://www.geometrictools.com/License/WildMagicLicense.pdf
//
// Version: 4.0.1 (2006/07/25)

#include "Wm4FoundationPCH.h"
#include "Wm4Vector2.h"
namespace Wm4 {

template<> const Vector2<float> Vector2<float>::ZERO(0.0f,0.0f);
template<> const Vector2<float> Vector2<float>::UNIT_X(1.0f,0.0f);
template<> const Vector2<float> Vector2<float>::UNIT_Y(0.0f,1.0f);
template<> const Vector2<float> Vector2<float>::ONE(1.0f,1.0f);

template<> const Vector2<double> Vector2<double>::ZERO(0.0,0.0);
template<> const Vector2<double> Vector2<double>::UNIT_X(1.0,0.0);
template<> const Vector2<double> Vector2<double>::UNIT_Y(0.0,1.0);
template<> const Vector2<double> Vector2<double>::ONE(1.0,1.0);
};