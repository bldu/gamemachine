﻿#include "stdafx.h"
#include "texture.h"
#include <linearmath.h>

Demo_Texture::~Demo_Texture()
{
	D(d);
	gm::GM_delete(d->demoWorld);
}

void Demo_Texture::init()
{
	D(d);
	Base::init();

	// 创建对象
	d->demoWorld = new gm::GMDemoGameWorld();

	// 创建一个纹理
	struct _ShaderCb : public gm::IPrimitiveCreatorShaderCallback
	{
		gm::GMDemoGameWorld* world = nullptr;

		_ShaderCb(gm::GMDemoGameWorld* d) : world(d)
		{
		}

		virtual void onCreateShader(gm::GMShader& shader) override
		{
			shader.setCull(gm::GMS_Cull::CULL);
			shader.getMaterial().kd = glm::vec3(1, 1, 1);

			auto pk = gm::GameMachine::instance().getGamePackageManager();
			auto& container = world->getAssets();

			gm::ITexture* tex = nullptr;
			gm::GMTextureUtil::createTexture("gamemachine.png", &tex);
			gm::GMTextureUtil::addTextureToShader(shader, tex, gm::GMTextureType::DIFFUSE);
			world->getAssets().insertAsset(gm::GMAssetType::Texture, tex);
		}
	} cb(d->demoWorld);

	// 创建一个带纹理的对象
	gm::GMfloat extents[] = { 1.f, .5f, .5f };
	gm::GMfloat pos[] = { 0, 0, 1 };
	gm::GMModel* model;
	gm::GMPrimitiveCreator::createQuad(extents, pos, &model, &cb);
	gm::GMAsset quadAsset = d->demoWorld->getAssets().insertAsset(gm::GMAssetType::Model, model);
	gm::GMGameObject* obj = new gm::GMGameObject(quadAsset);
	d->demoWorld->addObject("texture", obj);
}

void Demo_Texture::event(gm::GameMachineEvent evt)
{
	D(d);
	Base::event(evt);
	switch (evt)
	{
	case gm::GameMachineEvent::FrameStart:
		break;
	case gm::GameMachineEvent::FrameEnd:
		break;
	case gm::GameMachineEvent::Simulate:
		d->demoWorld->simulateGameWorld();
		break;
	case gm::GameMachineEvent::Render:
		d->demoWorld->renderScene();
		break;
	case gm::GameMachineEvent::Activate:
	{
		gm::IInput* inputManager = GM.getMainWindow()->getInputMananger();
		gm::IKeyboardState& kbState = inputManager->getKeyboardState();
		if (kbState.keyTriggered('N'))
			GMSetDebugState(DRAW_NORMAL, (GMGetDebugState(DRAW_NORMAL) + 1) % gm::GMStates_DebugOptions::DRAW_NORMAL_END);
		d->demoWorld->notifyControls();
		break;
	}
	case gm::GameMachineEvent::Deactivate:
		break;
	case gm::GameMachineEvent::Terminate:
		break;
	default:
		break;
	}
}
