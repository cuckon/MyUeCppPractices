// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

//#include "Math/Vector2D.h"

/**
 * The types of stylus inputs that can be potentially supported by a stylus.
 */
enum class DLLEXPORT EStylusInputType
{
	Position,
	Z,
	Pressure,
	Tilt,
	TangentPressure,
	ButtonPressure,
	Twist,
	Size
};
