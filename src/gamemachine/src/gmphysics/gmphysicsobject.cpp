﻿#include "stdafx.h"
#include "gmphysicsobject.h"
#include "gmbulletincludes.h"
#include <gmgameobject.h>
#include "gmphysicsshape.h"
#include "gmbullethelper.h"

const GMMotionStates& GMPhysicsObject::getMotionStates()
{
	D(d);
	return d->motionStates;
}

GMRigidPhysicsObject::~GMRigidPhysicsObject()
{
	D(d);
	GM_delete(d->motionState);
	if (!d->bodyDetached)
		GM_delete(d->body);
}

bool GMRigidPhysicsObject::isStaticObject() const
{
	D(d);
	GM_ASSERT(d->body);
	return d->body->isStaticObject();
}

bool GMRigidPhysicsObject::isKinematicObject() const
{
	D(d);
	GM_ASSERT(d->body);
	return d->body->isKinematicObject();
}

bool GMRigidPhysicsObject::isStaticOrKinematicObject() const
{
	D(d);
	GM_ASSERT(d->body);
	return d->body->isStaticOrKinematicObject();
}

bool GMRigidPhysicsObject::hasContactResponse() const
{
	D(d);
	GM_ASSERT(d->body);
	return d->body->hasContactResponse();
}

GMPhysicsActivationState GMRigidPhysicsObject::getActivationState()
{
	D(d);
	GM_ASSERT(d->body);
	return (GMPhysicsActivationState)d->body->getActivationState();
}

void GMRigidPhysicsObject::setActivationState(GMPhysicsActivationState state, bool force)
{
	D(d);
	GM_ASSERT(d->body);
	if (force)
		d->body->forceActivationState((GMint)state);
	else
		d->body->setActivationState((GMint)state);
}

void GMRigidPhysicsObject::activate(bool force)
{
	D(d);
	GM_ASSERT(d->body);
	d->body->activate(force);
}

glm::mat4 GMRigidPhysicsObject::getCenterOfMassTransform()
{
	D(d);
	GM_ASSERT(d->body);
	glm::mat4 val;
	d->body->getCenterOfMassTransform().getOpenGLMatrix(glm::value_ptr(val));
	return val;
}

glm::mat4 GMRigidPhysicsObject::getCenterOfMassTransformInversed()
{
	D(d);
	GM_ASSERT(d->body);
	glm::mat4 val;
	d->body->getCenterOfMassTransform().inverse().getOpenGLMatrix(glm::value_ptr(val));
	return val;
}

const GMMotionStates& GMRigidPhysicsObject::getMotionStates()
{
	D(d);
	D_BASE(db, Base);
	if (d->updateRevision != d->body->getUpdateRevisionInternal())
	{
		d->updateRevision = d->body->getUpdateRevisionInternal();
		d->body->getWorldTransform().getOpenGLMatrix(glm::value_ptr(db->motionStates.transform));

		btVector3 lv = d->body->getLinearVelocity();
		db->motionStates.linearVelocity = glm::vec3(lv[0], lv[1], lv[2]);
	}
	return db->motionStates;
}

void GMRigidPhysicsObject::setShape(GMAsset shape)
{
	D(d);
	D_BASE(db, Base);
	GM_ASSERT(!d->shape);
	d->shape = GMAssets::getPhysicsShape(shape);

	const glm::mat4& translation = db->gameObject->getTranslation();
	btTransform trans;
	trans.setFromOpenGLMatrix(glm::value_ptr(translation));
	initRigidBody(
		d->mass,
		trans,
		glm::vec3(1, 0, 0)
	);
}

void GMRigidPhysicsObject::initRigidBody(GMfloat mass, const btTransform& startTransform, const glm::vec3& color)
{
	D(d);
	btCollisionShape* shape = d->shape->getBulletShape();
	bool isDynamic = (mass != 0.f);

	btVector3 localInertia(0, 0, 0);
	if (isDynamic)
		shape->calculateLocalInertia(mass, localInertia);

	d->motionState = new btDefaultMotionState(startTransform);
	btRigidBody::btRigidBodyConstructionInfo cInfo(mass, d->motionState, shape, localInertia);

	d->body = new btRigidBody(cInfo);

	// 将GM对象与此bullet对象绑定
	d->body->setUserPointer(this);
}

void GMRigidPhysicsObject::detachRigidBody()
{
	D(d);
	d->bodyDetached = true;
}