﻿#include "stdafx.h"
#include "gmlight.h"

bool GMLight::setLightAttribute3(GMLightAttribute attr, GMfloat value[3])
{
	D(d);
	switch (attr)
	{
	case Position:
	{
		d->position[0] = value[0];
		d->position[1] = value[1];
		d->position[2] = value[2];
		d->position[3] = 1.0f;
		break;
	}
	case Color:
	{
		d->color[0] = value[0];
		d->color[1] = value[1];
		d->color[2] = value[2];
		d->color[3] = 1.0f;
		break;
	}
	default:
		return false;
	}
	return true;
}

bool GMLight::setLightAttribute(GMLightAttribute attr, GMfloat value)
{
	D(d);
	switch (attr)
	{
	case AttenuationConstant:
		d->attenuation.constant = value;
		break;
	case AttenuationLinear:
		d->attenuation.linear = value;
		break;
	case AttenuationExp:
		d->attenuation.exp = value;
		break;
	default:
		return false;
	}
	return true;
}
