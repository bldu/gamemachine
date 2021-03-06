﻿#include "stdafx.h"
#include "gmdx11light.h"
#include "gmdx11techniques.h"
#include "gmengine/gmlight_p.h"

BEGIN_NS

GM_PRIVATE_OBJECT_UNALIGNED(GMDx11Light)
{
	ID3DX11Effect* effect = nullptr;
	ID3DX11EffectVariable* lightAttributes = nullptr;
};

GMDx11Light::GMDx11Light()
{
	GM_CREATE_DATA();
}

GMDx11Light::~GMDx11Light()
{

}

void GMDx11Light::activateLight(GMuint32 index, ITechnique* technique)
{
	D(d);
	D_BASE(db, GMLight);
	if (!d->effect)
	{
		GMDx11Technique* dxTechnique = gm_cast<GMDx11Technique*>(technique);
		d->effect = dxTechnique->getEffect();
		GM_ASSERT(d->effect);
	}

#if GM_DEBUG
	GMDx11Technique* dxTechnique = gm_cast<GMDx11Technique*>(technique);
	GM_ASSERT(d->effect == dxTechnique->getEffect());
#endif

	if (!d->lightAttributes)
	{
		d->lightAttributes = d->effect->GetVariableByName("GM_LightAttributes");
	}

	ID3DX11EffectVariable* lightStruct = d->lightAttributes->GetElement(index);
	GM_ASSERT(lightStruct->IsValid());

	ID3DX11EffectVectorVariable* position = lightStruct->GetMemberByName("Position")->AsVector();
	GM_DX_TRY(position, position->SetFloatVector(db->position));

	ID3DX11EffectVectorVariable* color = lightStruct->GetMemberByName("Color")->AsVector();
	GM_DX_TRY(color, color->SetFloatVector(db->color));

	ID3DX11EffectVectorVariable* ambientIntensity = lightStruct->GetMemberByName("AmbientIntensity")->AsVector();
	GM_DX_TRY(ambientIntensity, ambientIntensity->SetFloatVector(db->ambientIntensity));

	ID3DX11EffectVectorVariable* diffuseIntensity = lightStruct->GetMemberByName("DiffuseIntensity")->AsVector();
	GM_DX_TRY(diffuseIntensity, diffuseIntensity->SetFloatVector(db->diffuseIntensity));

	ID3DX11EffectScalarVariable* specularIntensity = lightStruct->GetMemberByName("SpecularIntensity")->AsScalar();
	GM_DX_TRY(specularIntensity, specularIntensity->SetFloat(db->specularIntensity));

	ID3DX11EffectScalarVariable* type = lightStruct->GetMemberByName("Type")->AsScalar();
	GM_DX_TRY(type, type->SetInt(getLightType()));

	ID3DX11EffectVariable* attenuation = lightStruct->GetMemberByName("Attenuation");

	ID3DX11EffectScalarVariable* attenuationConstant = attenuation->GetMemberByName("Constant")->AsScalar();
	GM_DX_TRY(attenuationConstant, attenuationConstant->SetFloat(db->attenuation.constant));

	ID3DX11EffectScalarVariable* attenuationLinear = attenuation->GetMemberByName("Linear")->AsScalar();
	GM_DX_TRY(attenuationLinear, attenuationLinear->SetFloat(db->attenuation.linear));

	ID3DX11EffectScalarVariable* attenuationExp = attenuation->GetMemberByName("Exp")->AsScalar();
	GM_DX_TRY(attenuationExp, attenuationExp->SetFloat(db->attenuation.exp));
}

GM_PRIVATE_OBJECT_UNALIGNED_FROM(GMDx11DirectionalLight, GMDirectionalLight_t)
{
};

GMDx11DirectionalLight::GMDx11DirectionalLight()
{
	GM_CREATE_DATA();
}

GMDx11DirectionalLight::~GMDx11DirectionalLight()
{

}

bool GMDx11DirectionalLight::setLightAttribute3(GMLightAttribute attr, GMfloat value[3])
{
	D(d);
	switch (attr)
	{
	case Direction:
		d->direction[0] = value[0];
		d->direction[1] = value[1];
		d->direction[2] = value[2];
		d->direction[3] = 1;
		break;
	default:
		return Base::setLightAttribute3(attr, value);
	}
	return true;
}

void GMDx11DirectionalLight::activateLight(GMuint32 index, ITechnique* tech)
{
	Base::activateLight(index, tech);

	D(d);
	D_BASE(db, Base);
	GM_ASSERT(db->lightAttributes);
	ID3DX11EffectVariable* lightStruct = db->lightAttributes->GetElement(index);

	ID3DX11EffectVectorVariable* direction = lightStruct->GetMemberByName("Direction")->AsVector();
	GM_DX_TRY(direction, direction->SetFloatVector(d->direction));
}

GM_PRIVATE_OBJECT_UNALIGNED_FROM(GMDx11Spotlight, GMSpotlight_t)
{
};

GMDx11Spotlight::GMDx11Spotlight()
{
	GM_CREATE_DATA();
}

GMDx11Spotlight::~GMDx11Spotlight()
{

}

bool GMDx11Spotlight::setLightAttribute(GMLightAttribute attr, GMfloat value)
{
	D(d);
	switch (attr)
	{
	case CutOff:
		d->cutOff = value;
		break;
	default:
		return Base::setLightAttribute(attr, value);
	}
	return true;
}

void GMDx11Spotlight::activateLight(GMuint32 index, ITechnique* tech)
{
	Base::activateLight(index, tech);

	D(d);
	D_BASE(db, GMDx11Light);
	GM_ASSERT(db->lightAttributes);
	ID3DX11EffectVariable* lightStruct = db->lightAttributes->GetElement(index);

	ID3DX11EffectScalarVariable* cutOff = lightStruct->GetMemberByName("CutOff")->AsScalar();
	GM_DX_TRY(cutOff, cutOff->SetFloat(Cos(Radians(d->cutOff))));
}

END_NS