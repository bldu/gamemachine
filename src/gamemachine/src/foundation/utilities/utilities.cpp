﻿#include "stdafx.h"
#include "utilities.h"
#include "foundation/gamemachine.h"

GMfloat* GMPrimitiveCreator::unitExtents()
{
	static GMfloat o[3] = { 1, 1, 1 };
	return o;
}

GMfloat* GMPrimitiveCreator::origin()
{
	static GMfloat o[3] = { 0 };
	return o;
}

void GMPrimitiveCreator::createCube(GMfloat extents[3], OUT GMModel** obj, IPrimitiveCreatorShaderCallback* shaderCallback, GMModelType type)
{
	static constexpr GMfloat v[24] = {
		-1, 1, -1,
		-1, -1, -1,
		1, 1, -1,
		1, -1, -1,
		-1, 1, 1,
		-1, -1, 1,
		1, 1, 1,
		1, -1, 1,
	};
	static constexpr GMint uv[] = {
		0, 1,
		0, 0,
		1, 0,

		0, 1,
		1, 0,
		1, 1,
	};
	static constexpr GMint indices[] = {
		0, 3, 1,
		0, 2, 3,

		6, 5, 7,
		6, 4, 5,

		4, 1, 5,
		4, 0, 1,

		2, 7, 3,
		2, 6, 7,

		1, 7, 5,
		1, 3, 7,

		4, 2, 0,
		4, 6, 2,
	};

	GMModel* model = new GMModel();

	// 实体
	GMfloat t[24];
	for (GMint i = 0; i < 24; ++i)
	{
		t[i] = extents[i % 3] * v[i];
	}

	{
		model->setType(type);
		GMMesh* body = model->getMesh();
		body->setArrangementMode(GMArrangementMode::Triangle_Strip);

		GMComponent* component = new GMComponent(body);
		GMFloat4 f4_vertex, f4_normal;
		for (GMint i = 0; i < 12; ++i)
		{
			component->beginFace();
			for (GMint j = 0; j < 3; ++j) // j表示面的一个顶点
			{
				GMint idx = i * 3 + j; //顶点的开始
				GMint idx_next = i * 3 + (j + 1) % 3;
				GMint idx_prev = i * 3 + (j + 2) % 3;
				GMVec3 vertex(t[indices[idx] * 3], t[indices[idx] * 3 + 1], t[indices[idx] * 3 + 2]);
				GMVec3 vertex_prev(t[indices[idx_prev] * 3], t[indices[idx_prev] * 3 + 1], t[indices[idx_prev] * 3 + 2]),
					vertex_next(t[indices[idx_next] * 3], t[indices[idx_next] * 3 + 1], t[indices[idx_next] * 3 + 2]);
				GMVec3 normal = Cross(vertex - vertex_prev, vertex_next - vertex);
				normal = FastNormalize(normal);

				vertex.loadFloat4(f4_vertex);
				normal.loadFloat4(f4_normal);
				component->vertex(f4_vertex[0], f4_vertex[1], f4_vertex[2]);
				component->normal(f4_normal[0], f4_normal[1], f4_normal[2]);
				component->uv(uv[(i % 2) * 6 + (j * 2)], uv[(i % 2) * 6 + (j * 2) + 1]);
				component->color(1.f, 1.f, 1.f);
			}
			component->endFace();
		}
		if (shaderCallback)
			shaderCallback->onCreateShader(component->getShader());
	}

	*obj = model;
}

void GMPrimitiveCreator::createQuad(GMfloat extents[3], GMfloat position[3], OUT GMModel** obj, IPrimitiveCreatorShaderCallback* shaderCallback, GMModelType type, GMCreateAnchor anchor, GMfloat (*customUV)[12])
{
	static constexpr GMfloat v_anchor_center[] = {
		-1, 1, 0,
		-1, -1, 0,
		1, -1, 0,
		1, 1, 0,
	};

	const GMfloat(*_customUV)[12] = customUV ? customUV : &v_anchor_center;
	const GMfloat(&uvArr)[12] = *_customUV;

	static constexpr GMfloat v_anchor_top_left[] = {
		0, 0, 0,
		0, -2, 0,
		2, -2, 0,
		2, 0, 0,
	};

	const GMfloat(&v)[12] = (anchor == TopLeft) ? v_anchor_top_left : v_anchor_center;

	static constexpr GMint indices[] = {
		0, 3, 1,
		2, 1, 3,
	};

	GMModel* model = new GMModel();

	// 实体
	GMfloat t[12];
	for (GMint i = 0; i < 12; ++i)
	{
		t[i] = extents[i % 3] * v[i] + position[i % 3];
	}

	{
		model->setType(type);
		GMMesh* body = model->getMesh();
		body->setArrangementMode(GMArrangementMode::Triangle_Strip);

		GMComponent* component = new GMComponent(body);
		GMFloat4 f4_vertex, f4_normal, f4_uv;
		for (GMint i = 0; i < 2; ++i)
		{
			component->beginFace();
			for (GMint j = 0; j < 3; ++j) // j表示面的一个顶点
			{
				GMint idx = i * 3 + j; //顶点的开始
				GMint idx_next = i * 3 + (j + 1) % 3;
				GMint idx_prev = i * 3 + (j + 2) % 3;
				GMVec2 uv(uvArr[indices[idx] * 3], uvArr[indices[idx] * 3 + 1]);
				GMVec3 vertex(t[indices[idx] * 3], t[indices[idx] * 3 + 1], t[indices[idx] * 3 + 2]);
				GMVec3 vertex_prev(t[indices[idx_prev] * 3], t[indices[idx_prev] * 3 + 1], t[indices[idx_prev] * 3 + 2]),
					vertex_next(t[indices[idx_next] * 3], t[indices[idx_next] * 3 + 1], t[indices[idx_next] * 3 + 2]);
				GMVec3 normal = Cross(vertex - vertex_prev, vertex_next - vertex);
				normal = FastNormalize(normal);

				vertex.loadFloat4(f4_vertex);
				normal.loadFloat4(f4_normal);
				uv.loadFloat4(f4_uv);
				component->vertex(f4_vertex[0], f4_vertex[1], f4_vertex[2]);
				component->normal(f4_normal[0], f4_normal[1], f4_normal[2]);
				if (customUV)
					component->uv(f4_uv[0], 1 - f4_uv[1]);
				else
					component->uv((f4_uv[0] + 1) / 2, 1 - (f4_uv[1] + 1) / 2);
				component->color(1.f, 1.f, 1.f);
			}
			component->endFace();
		}
		if (shaderCallback)
			shaderCallback->onCreateShader(component->getShader());
	}

	*obj = model;
}

void GMPrimitiveCreator::createQuad3D(GMfloat extents[3], GMfloat position[12], OUT GMModel** obj, IPrimitiveCreatorShaderCallback* shaderCallback, GMModelType type, GMfloat(*customUV)[8])
{
	static constexpr GMfloat defaultUV[] = {
		-1, 1,
		-1, -1,
		1, -1,
		1, 1,
	};

	const GMfloat(*_pos)[12] = (GMfloat(*)[12])(position);
	const GMfloat(*_uv)[8] = customUV ? customUV : (GMfloat(*)[8])defaultUV;
	const GMfloat(&uvArr)[8] = *_uv;
	const GMfloat(&v)[12] = *_pos;

	static constexpr GMint indices[] = {
		0, 3, 1,
		2, 1, 3,
	};

	GMModel* model = new GMModel();

	// 实体
	GMfloat t[12];
	for (GMint i = 0; i < 12; i++)
	{
		t[i] = extents[i % 3] * v[i];
	}

	{
		model->setType(type);
		GMMesh* body = model->getMesh();
		body->setArrangementMode(GMArrangementMode::Triangle_Strip);

		GMComponent* component = new GMComponent(body);
		GMFloat4 f4_vertex, f4_normal, f4_uv;
		for (GMint i = 0; i < 2; i++)
		{
			component->beginFace();
			for (GMint j = 0; j < 3; j++) // j表示面的一个顶点
			{
				GMint idx = i * 3 + j; //顶点的开始
				GMint idx_next = i * 3 + (j + 1) % 3;
				GMint idx_prev = i * 3 + (j + 2) % 3;
				GMVec2 uv(uvArr[indices[idx] * 2], uvArr[indices[idx] * 2 + 1]);
				GMVec3 vertex(t[indices[idx] * 3], t[indices[idx] * 3 + 1], t[indices[idx] * 3 + 2]);
				GMVec3 vertex_prev(t[indices[idx_prev] * 3], t[indices[idx_prev] * 3 + 1], t[indices[idx_prev] * 3 + 2]),
					vertex_next(t[indices[idx_next] * 3], t[indices[idx_next] * 3 + 1], t[indices[idx_next] * 3 + 2]);
				GMVec3 normal = Cross(vertex - vertex_prev, vertex_next - vertex);
				normal = FastNormalize(normal);

				vertex.loadFloat4(f4_vertex);
				normal.loadFloat4(f4_normal);
				uv.loadFloat4(f4_uv);
				component->vertex(f4_vertex[0], f4_vertex[1], f4_vertex[2]);
				component->normal(f4_normal[0], f4_normal[1], f4_normal[2]);
				if (customUV)
					component->uv(f4_uv[0], 1 - f4_uv[1]);
				else
					component->uv((f4_uv[0] + 1) / 2, 1 - (f4_uv[1] + 1) / 2);
				component->color(1.f, 1.f, 1.f);
			}
			component->endFace();
		}
		if (shaderCallback)
			shaderCallback->onCreateShader(component->getShader());
	}

	*obj = model;
}


//////////////////////////////////////////////////////////////////////////
void GMPrimitiveUtil::translateModelTo(REF GMModel& model, const GMfloat(&trans)[3])
{
	GMModelPainter* painter = model.getPainter();
	GMMesh* mesh = model.getMesh();
	painter->beginUpdateBuffer(mesh);
	GMfloat* buffer = reinterpret_cast<GMfloat*>(painter->getBuffer());
	GMint vertexCount = mesh->get_transferred_positions_byte_size() / (sizeof(GMfloat) * GMModel::PositionDimension);

	// 计算首个偏移
	GMfloat* first = buffer;
	GMfloat delta[] = {
		trans[0] - first[0],
		trans[1] - first[1],
		trans[2] - first[2]
	};

	for (GMint v = 0; v < vertexCount; ++v)
	{
		GMfloat* ptr = buffer + v * GMModel::PositionDimension;
		ptr[0] += delta[0];
		ptr[1] += delta[1];
		ptr[2] += delta[2];
	}
	painter->endUpdateBuffer();
}

void GMPrimitiveUtil::scaleModel(REF GMModel& model, const GMfloat (&scaling)[3])
{
	GMModelPainter* painter = model.getPainter();
	GMMesh* mesh = model.getMesh();
	painter->beginUpdateBuffer(mesh);
	GMfloat* buffer = reinterpret_cast<GMfloat*>(painter->getBuffer());
	GMint vertexCount = mesh->get_transferred_positions_byte_size() / (sizeof(GMfloat) * GMModel::PositionDimension);
	for (GMint v = 0; v < vertexCount; ++v)
	{
		GMfloat* ptr = buffer + v * GMModel::PositionDimension;
		ptr[0] *= scaling[0];
		ptr[1] *= scaling[1];
		ptr[2] *= scaling[2];
	}
	painter->endUpdateBuffer();
}

void GMTextureUtil::createTexture(const GMString& filename, ITexture** texture)
{
	gm::GMImage* img = nullptr;
	gm::GMBuffer buf;
	GM.getGamePackageManager()->readFile(gm::GMPackageIndex::Textures, filename, &buf);
	gm::GMImageReader::load(buf.buffer, buf.size, &img);
	GM_ASSERT(img);

	GM.getFactory()->createTexture(img, texture);
	GM_ASSERT(texture);
	gm::GM_delete(img);
}

void GMTextureUtil::addTextureToShader(gm::GMShader& shader, ITexture* texture, GMTextureType type)
{
	auto& frames = shader.getTexture().getTextureFrames(type, 0);
	frames.addFrame(texture);
}