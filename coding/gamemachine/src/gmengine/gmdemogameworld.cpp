﻿#include "stdafx.h"
#include "gmdemogameworld.h"
#include "foundation/gamemachine.h"
#include "gmcharacter.h"

GMDemoGameWorld::GMDemoGameWorld()
{
	D(d);
	setMajorCharacter(new GMCharacter(0));
	GameMachine::instance().getGraphicEngine()->setCurrentWorld(this);
}

GMDemoGameWorld::~GMDemoGameWorld()
{
	D(d);
	for (auto& object : d->renderList)
	{
		delete object.second;
	}
}

void GMDemoGameWorld::renderGameWorld()
{
	D(d);
	GameMachine::instance().getGraphicEngine()->newFrame();
	IGraphicEngine* engine = GameMachine::instance().getGraphicEngine();
	for (auto& object : d->renderList)
	{
		engine->drawObject(object.second);
	}
}

GMPhysicsWorld* GMDemoGameWorld::physicsWorld()
{
	return nullptr;
}

bool GMDemoGameWorld::appendObject(const GMString& str, GMGameObject* obj)
{
	D(d);
	auto& r = d->renderList.find(str);
	if (r != d->renderList.end())
		return false;
	d->renderList[str] = obj;
	return true;
}

void GMDemoGameWorld::createCube(GMfloat extents[3], OUT GMGameObject** obj)
{
	static CONST_EXPR GMfloat v[24] = {
		1, -1, 1,
		1, -1, -1,
		-1, -1, 1,
		-1, -1, -1,
		1, 1, 1,
		1, 1, -1,
		-1, 1, 1,
		-1, 1, -1,
	};
	static CONST_EXPR GMint indices[] = {
		0, 2, 1,
		2, 3, 1,
		4, 5, 6,
		6, 5, 7,
		0, 1, 4,
		1, 5, 4,
		2, 6, 3,
		3, 6, 7,
		0, 4, 2,
		2, 4, 6,
		1, 3, 5,
		3, 7, 5,
	};

	Object* coreObj = new Object();
	GMMesh* child = new GMMesh();
	child->setArrangementMode(GMArrangementMode::Triangle_Strip);

	GMfloat t[24];
	for (GMint i = 0; i < 24; i++)
	{
		t[i] = extents[i % 3] * v[i];
	}

	Component* component = new Component(child);

	linear_math::Vector3 normal;
	for (GMint i = 0; i < 12; i++)
	{
		component->beginFace();
		for (GMint j = 0; j < 3; j++) // j表示面的一个顶点
		{
			GMint idx = i * 3 + j; //顶点的开始
			GMint idx_next = i * 3 + (j + 1) % 3;
			GMint idx_prev = i * 3 + (j + 2) % 3;
			linear_math::Vector3 vertex(t[indices[idx] * 3], t[indices[idx] * 3 + 1], t[indices[idx] * 3 + 2]);
			linear_math::Vector3 vertex_prev(t[indices[idx_prev] * 3], t[indices[idx_prev] * 3 + 1], t[indices[idx_prev] * 3 + 2]),
				vertex_next(t[indices[idx_next] * 3], t[indices[idx_next] * 3 + 1], t[indices[idx_next] * 3 + 2]);
			linear_math::Vector3 normal = linear_math::cross(vertex - vertex_prev, vertex_next - vertex);
			normal = linear_math::normalize(normal);

			component->vertex(vertex[0], vertex[1], vertex[2]);
			component->normal(normal[0], normal[1], normal[2]);
			//TODO
			//component->uv(vertex.decalS, vertex.decalT);
			//component->lightmap(1.f, 1.f);
		}
		component->endFace();
	}

	child->appendComponent(component);
	coreObj->append(child);

	GMGameObject* gameObject = new GMGameObject(coreObj);
	*obj = gameObject;
	GameMachine::instance().initObjectPainter(gameObject);
}