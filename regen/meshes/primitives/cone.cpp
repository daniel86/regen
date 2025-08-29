#include "cone.h"

using namespace regen;

Cone::Cone(GLenum primitive, const BufferUpdateFlags &hints)
		: Mesh(primitive, hints) {
	pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	nor_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
}

/////////////////
/////////////////

ConeOpened::Config::Config()
		: cosAngle(0.5),
		  height(1.0f),
		  isNormalRequired(GL_TRUE),
		  levelOfDetails({1}) {
}

ConeOpened::ConeOpened(const Config &cfg)
		: Cone(GL_TRIANGLE_FAN, cfg.updateHint) {
	setBufferMapMode(cfg.mapMode);
	setClientAccessMode(cfg.accessMode);
	updateAttributes(cfg);
}

void ConeOpened::generateLODLevel(const Config &cfg,
								  GLuint lodLevel,
								  GLuint vertexOffset,
								  GLuint indexOffset) {
	// map client data for writing
	auto v_pos = (Vec3f*) pos_->clientBuffer()->clientData(0);
	auto v_nor = (cfg.isNormalRequired ?
				  (Vec3f*) nor_->clientBuffer()->clientData(0) : nullptr);

	GLfloat phi = acos(cfg.cosAngle);
	GLfloat radius = tan(phi) * cfg.height;
	GLfloat angle = 0.0f;
	GLfloat angleStep = 2.0f * M_PI / (GLfloat) lodLevel;
	GLuint i = vertexOffset;

	v_pos[i] = Vec3f(0.0f);
	if (cfg.isNormalRequired) {
		v_nor[i] = Vec3f(0.0f, -1.0f, 0.0f);
	}

	for (; i < lodLevel + 1; ++i) {
		angle += angleStep;
		GLfloat s = sin(angle) * radius;
		GLfloat c = cos(angle) * radius;
		Vec3f pos(c, s, cfg.height);
		v_pos[i + 1] = pos;
		minPosition_.setMin(pos);
		maxPosition_.setMax(pos);
		if (cfg.isNormalRequired) {
			Vec3f n(c, 0.0, s);
			n.normalize();
			v_nor[i + 1] = n;
		}
	}
}

void ConeOpened::updateAttributes(const Config &cfg) {
	std::vector<GLuint> LODs;
	GLuint vertexOffset = 0;
	for (auto &lod: cfg.levelOfDetails) {
		GLuint lodLevel = 4u * pow(lod, 2);
		LODs.push_back(lodLevel);
		auto &x = meshLODs_.emplace_back();
		x.d->numVertices = lodLevel + 2;
		x.d->vertexOffset = vertexOffset;
		vertexOffset += x.d->numVertices;
	}
	GLuint numVertices = vertexOffset;

	minPosition_ = Vec3f(0.0);
	maxPosition_ = Vec3f(0.0);
	pos_->setVertexData(numVertices);
	if (cfg.isNormalRequired) {
		nor_->setVertexData(numVertices);
	}

	for (auto i = 0u; i < LODs.size(); ++i) {
		generateLODLevel(cfg,
						 LODs[i],
						 meshLODs_[i].d->vertexOffset,
						 meshLODs_[i].d->indexOffset);
	}

	begin(INTERLEAVED);
	setInput(pos_);
	if (cfg.isNormalRequired)
		setInput(nor_);
	end();

	activateLOD(0);
}

/////////////////
/////////////////

ConeClosed::Config::Config()
		: radius(0.5),
		  height(1.0f),
		  isNormalRequired(GL_TRUE),
		  isBaseRequired(GL_TRUE),
		  levelOfDetails({1}) {
}

ref_ptr<Mesh> ConeClosed::getBaseCone() {
	static ref_ptr<ConeClosed> mesh;
	if (mesh.get() == nullptr) {
		Config cfg;
		cfg.height = 1.0f;
		cfg.radius = 0.5;
		cfg.levelOfDetails = {3, 2, 1};
		cfg.isNormalRequired = GL_FALSE;
		cfg.isBaseRequired = GL_TRUE;
		mesh = ref_ptr<ConeClosed>::alloc(cfg);
	}
	return ref_ptr<Mesh>::alloc(mesh);
}

ConeClosed::ConeClosed(const Config &cfg)
		: Cone(GL_TRIANGLES, cfg.updateHint) {
	setBufferMapMode(cfg.mapMode);
	setClientAccessMode(cfg.accessMode);
	updateAttributes(cfg);
}

static void loadConeData(
		Vec3f *pos, Vec3f *nor,
		Vec3f &min,
		Vec3f &max,
		GLboolean useBase,
		GLuint subdivisions,
		GLfloat radius,
		GLfloat height) {
	GLfloat angle = 0.0f;
	GLfloat angleStep = 2.0f * M_PI / (GLfloat) subdivisions;
	GLint i = 0;

	// apex
	pos[i] = Vec3f(0.0f);
	if (nor) nor[i] = Vec3f(0.0f, 0.0f, -1.0f);
	++i;
	// base center
	if (useBase) {
		pos[i] = Vec3f(0.0f, 0.0f, height);
		min.setMin(pos[i]);
		max.setMax(pos[i]);
		if (nor) nor[i] = Vec3f(0.0f, 0.0f, 1.0f);
		++i;
	}

	GLint numVertices = subdivisions + i;
	for (; i < numVertices; ++i) {
		angle += angleStep;
		GLfloat s = sin(angle) * radius;
		GLfloat c = cos(angle) * radius;
		pos[i] = Vec3f(c, s, height);
		min.setMin(pos[i]);
		max.setMax(pos[i]);
		if (nor) {
			nor[i] = Vec3f(c, s, 0.0);
			nor[i].normalize();
		}
	}
}

void ConeClosed::generateLODLevel(
		const Config &cfg,
		GLuint lodLevel,
		GLuint vertexOffset,
		GLuint indexOffset) {
	// map client data for writing
	auto v_pos = (Vec3f*) pos_->clientBuffer()->clientData(0);
	auto v_nor = (cfg.isNormalRequired ?
				  (Vec3f*) nor_->clientBuffer()->clientData(0) : nullptr);
	auto indices = (byte*)indices_->clientBuffer()->clientData(0);
	auto indexType = indices_->baseType();

	// create cone vertex data
	loadConeData(
			v_pos+vertexOffset,
			(v_nor ? v_nor+vertexOffset : v_nor),
			minPosition_, maxPosition_,
			cfg.isBaseRequired, lodLevel,
			cfg.radius, cfg.height);

	// create cone index data
	const GLuint apexIndex = vertexOffset;
	const GLuint baseCenterIndex = vertexOffset + 1;
	GLuint faceIndex = indexOffset;
	GLint vIndex = vertexOffset + cfg.isBaseRequired ? 2 : 1;
	// cone
	for (GLuint i = 0; i < lodLevel; ++i) {
		setIndexValue(indices, indexType, faceIndex++, apexIndex);
		setIndexValue(indices, indexType, faceIndex++, (i + 1 == lodLevel ? vIndex : vIndex + i + 1));
		setIndexValue(indices, indexType, faceIndex++, vIndex + i);
	}
	// base
	if (cfg.isBaseRequired) {
		for (GLuint i = 0; i < lodLevel; ++i) {
			setIndexValue(indices, indexType, faceIndex++, baseCenterIndex);
			setIndexValue(indices, indexType, faceIndex++, vIndex + i);
			setIndexValue(indices, indexType, faceIndex++, (i + 1 == lodLevel ? vIndex : vIndex + i + 1));
		}
	}
}

void ConeClosed::updateAttributes(const Config &cfg) {
	std::vector<GLuint> LODs;
	GLuint vertexOffset = 0;
	GLuint indexOffset = 0;
	for (auto &lod: cfg.levelOfDetails) {
		GLuint lodLevel = 4u * pow(lod, 2);
		LODs.push_back(lodLevel);
		auto &x = meshLODs_.emplace_back();
		x.d->numVertices = lodLevel + 1;
		if (cfg.isBaseRequired) x.d->numVertices += 1;
		x.d->vertexOffset = vertexOffset;
		vertexOffset += x.d->numVertices;

		GLuint numFaces = lodLevel;
		if (cfg.isBaseRequired) { numFaces *= 2; }
		x.d->numIndices = numFaces * 3;
		x.d->indexOffset = indexOffset;
		indexOffset += x.d->numIndices;
	}
	GLuint numVertices = vertexOffset;
	GLuint numIndices = indexOffset;

	minPosition_ = Vec3f(0.0);
	maxPosition_ = Vec3f(0.0);
	pos_->setVertexData(numVertices);
	if (cfg.isNormalRequired) {
		nor_->setVertexData(numVertices);
	}
	indices_ = createIndexInput(numIndices, numVertices);

	for (auto i = 0u; i < LODs.size(); ++i) {
		generateLODLevel(cfg,
						 LODs[i],
						 meshLODs_[i].d->vertexOffset,
						 meshLODs_[i].d->indexOffset);
	}

	begin(INTERLEAVED);
	auto indexRef = setIndices(indices_, numVertices);
	setInput(pos_);
	if (cfg.isNormalRequired)
		setInput(nor_);
	end();

	for (auto &x: meshLODs_) {
		// add the index buffer offset (in number of bytes)
		x.d->indexOffset = indexRef->address() + x.d->indexOffset * indices_->dataTypeBytes();
	}
	activateLOD(0);
}
