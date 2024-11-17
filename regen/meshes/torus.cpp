#include "tessellation.h"
#include "torus.h"

using namespace regen;

namespace regen {
	std::ostream &operator<<(std::ostream &out, const Torus::TexcoMode &mode) {
		switch (mode) {
			case Torus::TEXCO_MODE_NONE:
				return out << "NONE";
			case Torus::TEXCO_MODE_UV:
				return out << "UV";
			case Torus::TEXCO_MODE_CUBE_MAP:
				return out << "CUBE_MAP";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, Torus::TexcoMode &mode) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "NONE") mode = Torus::TEXCO_MODE_NONE;
		else if (val == "UV") mode = Torus::TEXCO_MODE_UV;
		else if (val == "CUBE_MAP") mode = Torus::TEXCO_MODE_CUBE_MAP;
		else {
			REGEN_WARN("Unknown box texco mode '" << val << "'. Using NONE texco.");
			mode = Torus::TEXCO_MODE_NONE;
		}
		return in;
	}
}

ref_ptr<Torus> Torus::getUnitTorus() {
	static ref_ptr<Torus> mesh;
	if (mesh.get() == nullptr) {
		Config cfg;
		cfg.posScale = Vec3f(1.0f);
		cfg.rotation = Vec3f(0.0, 0.0f, 0.0f);
		cfg.texcoMode = TEXCO_MODE_NONE;
		cfg.isNormalRequired = GL_FALSE;
		cfg.isTangentRequired = GL_FALSE;
		cfg.usage = VBO::USAGE_STATIC;
		mesh = ref_ptr<Torus>::alloc(cfg);
		return mesh;
	} else {
		return ref_ptr<Torus>::alloc(mesh);
	}
}

Torus::Torus(const Config &cfg)
		: Mesh(GL_TRIANGLES, cfg.usage) {
	pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	nor_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	tan_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
	indices_ = ref_ptr<ShaderInput1ui>::alloc("i");
	updateAttributes(cfg);
}

Torus::Torus(const ref_ptr<Torus> &other)
		: Mesh(other) {
	pos_ = ref_ptr<ShaderInput3f>::dynamicCast(
			inputContainer_->getInput(ATTRIBUTE_NAME_POS));
	nor_ = ref_ptr<ShaderInput3f>::dynamicCast(
			inputContainer_->getInput(ATTRIBUTE_NAME_NOR));
	tan_ = ref_ptr<ShaderInput4f>::dynamicCast(
			inputContainer_->getInput(ATTRIBUTE_NAME_TAN));
	indices_ = ref_ptr<ShaderInput1ui>::dynamicCast(
			inputContainer_->getInput("i"));
}

Torus::Config::Config()
		: levelOfDetail(0),
		  posScale(Vec3f(1.0f)),
		  rotation(Vec3f(0.0f)),
		  texcoScale(Vec2f(1.0f)),
		  texcoMode(TEXCO_MODE_UV),
		  isNormalRequired(GL_TRUE),
		  isTangentRequired(GL_FALSE),
		  usage(VBO::USAGE_DYNAMIC),
		  ringRadius(1.0f),
		  tubeRadius(0.5f) {
}

void Torus::updateAttributes(const Config &cfg) {
    // Generate torus vertices
    const GLuint levelOfDetail = pow(4u, 1 + cfg.levelOfDetail);
    const GLuint numVertices = (levelOfDetail + 1) * (levelOfDetail + 1);
    const GLuint numIndices = levelOfDetail * levelOfDetail * 6;
    pos_->setVertexData(numVertices);
    if (cfg.isNormalRequired) {
        nor_->setVertexData(numVertices);
    }
    if (cfg.isTangentRequired) {
        tan_->setVertexData(numVertices);
    }
    if (cfg.texcoMode == TEXCO_MODE_UV) {
        texco_ = ref_ptr<ShaderInput2f>::alloc("texco0");
        texco_->setVertexData(numVertices);
    } else if (cfg.texcoMode == TEXCO_MODE_CUBE_MAP) {
        texco_ = ref_ptr<ShaderInput3f>::alloc("texco0");
        texco_->setVertexData(numVertices);
    }

    const float ringStep = 2.0f * M_PI / levelOfDetail;
    const float tubeStep = 2.0f * M_PI / levelOfDetail;

    GLuint vertexIndex = 0;
    for (GLuint i = 0; i <= levelOfDetail; ++i) {
        float theta = i * ringStep;
        float cosTheta = cos(theta);
        float sinTheta = sin(theta);

        for (GLuint j = 0; j <= levelOfDetail; ++j) {
            float phi = j * tubeStep;
            float cosPhi = cos(phi);
            float sinPhi = sin(phi);

            Vec3f pos(
                (cfg.ringRadius + cfg.tubeRadius * cosPhi) * cosTheta,
                cfg.tubeRadius * sinPhi,
                (cfg.ringRadius + cfg.tubeRadius * cosPhi) * sinTheta
            );

            pos = cfg.posScale * pos;

            pos_->setVertex(vertexIndex, pos);

            if (cfg.isNormalRequired) {
                nor_->setVertex(vertexIndex, Vec3f(
                		cosPhi * cosTheta,
                		sinPhi,
                		cosPhi * sinTheta));
            }

            if (cfg.texcoMode == TEXCO_MODE_UV) {
                Vec2f texco((float)i / levelOfDetail, (float)j / levelOfDetail);
                ((ShaderInput2f*)texco_.get())->setVertex(vertexIndex, texco * cfg.texcoScale);
            } else if (cfg.texcoMode == TEXCO_MODE_CUBE_MAP) {
                Vec3f texco = pos;
                texco.normalize();
				((ShaderInput3f*)texco_.get())->setVertex(vertexIndex, texco);
            }

            if (cfg.isTangentRequired) {
                tan_->setVertex(vertexIndex, Vec4f(
                		-sinTheta,
                		0.0f,
                		cosTheta, 1.0f));
            }

            ++vertexIndex;
        }
    }

    // Generate indices
    indices_->setVertexData(numIndices);
    auto *indices = (GLuint *) indices_->clientDataPtr();
    GLuint index = 0;
    for (GLuint i = 0; i < levelOfDetail; ++i) {
        for (GLuint j = 0; j < levelOfDetail; ++j) {
            GLuint first = (i * (levelOfDetail + 1)) + j;
            GLuint second = first + levelOfDetail + 1;

            indices[index++] = first;
            indices[index++] = first + 1;
            indices[index++] = second;

            indices[index++] = second;
            indices[index++] = first + 1;
            indices[index++] = second + 1;
        }
    }

    // Set up the vertex attributes
    begin(ShaderInputContainer::INTERLEAVED);
	setIndices(indices_, numVertices);
    setInput(pos_);
    if (cfg.isNormalRequired) {
        setInput(nor_);
    }
    if (cfg.texcoMode != TEXCO_MODE_NONE) {
        setInput(texco_);
    }
    if (cfg.isTangentRequired) {
        setInput(tan_);
    }
    end();

    minPosition_ = -cfg.posScale;
    maxPosition_ = cfg.posScale;
}
