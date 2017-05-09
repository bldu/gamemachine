﻿#ifndef __GLYPHOBJECT_H__
#define __GLYPHOBJECT_H__
#include "common.h"
#include "gameobject.h"
BEGIN_NS

struct GlyphProperties
{
};

struct GlyphObjectPrivate
{
	std::wstring lastRenderText;
	std::wstring text;
	GlyphProperties properties;
	GMfloat left, bottom, width, height;
	ITexture* texture;
};

class Component;
class GlyphObject : public GameObject
{
	DEFINE_PRIVATE(GlyphObject)

public:
	GlyphObject();

public:
	void setText(const GMWChar* text);
	void setGeometry(GMfloat left, GMfloat bottom, GMfloat width, GMfloat height);
	void updateObject();

private:
	virtual void getReadyForRender(DrawingList& list) override;
	virtual void onAppendingObjectToWorld() override;

private:
	void constructObject();
};

END_NS
#endif