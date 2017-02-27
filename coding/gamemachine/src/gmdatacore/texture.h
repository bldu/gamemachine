﻿#ifndef __TEXTURE_H__
#define __TEXTURE_H__
#include "common.h"
BEGIN_NS

struct TextureInfo;
struct ITexture
{
	virtual ~ITexture();
	virtual void beginTexture(TextureInfo* ti) = 0;
	virtual void endTexture() = 0;
};

END_NS
#endif