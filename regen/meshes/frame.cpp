#include "tessellation.h"
#include "frame.h"

using namespace regen;

// TODO: load frame in different levels of detail

namespace regen {
	std::ostream &operator<<(std::ostream &out, const FrameMesh::TexcoMode &mode) {
		switch (mode) {
			case FrameMesh::TEXCO_MODE_NONE:
				return out << "NONE";
			case FrameMesh::TEXCO_MODE_UV:
				return out << "UV";
		}
		return out;
	}

	std::istream &operator>>(std::istream &in, FrameMesh::TexcoMode &mode) {
		std::string val;
		in >> val;
		boost::to_upper(val);
		if (val == "NONE") mode = FrameMesh::TEXCO_MODE_NONE;
		else if (val == "UV") mode = FrameMesh::TEXCO_MODE_UV;
		else {
			REGEN_WARN("Unknown texco mode '" << val << "'. Using NONE texco.");
			mode = FrameMesh::TEXCO_MODE_NONE;
		}
		return in;
	}
}

ref_ptr<FrameMesh> FrameMesh::getUnitFrame() {
	static ref_ptr<FrameMesh> mesh;
	if (mesh.get() == nullptr) {
		Config cfg;
		cfg.posScale = Vec3f(1.0f);
		cfg.rotation = Vec3f(0.0, 0.0f, 0.0f);
		cfg.texcoMode = TEXCO_MODE_NONE;
		cfg.isNormalRequired = GL_FALSE;
		cfg.isTangentRequired = GL_FALSE;
		cfg.borderSize = 0.1f;
		cfg.usage = VBO::USAGE_STATIC;
		mesh = ref_ptr<FrameMesh>::alloc(cfg);
		return mesh;
	} else {
		return ref_ptr<FrameMesh>::alloc(mesh);
	}
}

FrameMesh::FrameMesh(const Config &cfg)
		: Mesh(GL_TRIANGLES, cfg.usage) {
	pos_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_POS);
	nor_ = ref_ptr<ShaderInput3f>::alloc(ATTRIBUTE_NAME_NOR);
	tan_ = ref_ptr<ShaderInput4f>::alloc(ATTRIBUTE_NAME_TAN);
	indices_ = ref_ptr<ShaderInput1ui>::alloc("i");
	updateAttributes(cfg);
}

FrameMesh::FrameMesh(const ref_ptr<FrameMesh> &other)
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

FrameMesh::Config::Config()
		: levelOfDetail(0),
		  posScale(Vec3f(1.0f)),
		  rotation(Vec3f(0.0f)),
		  texcoScale(Vec2f(1.0f)),
		  texcoMode(TEXCO_MODE_UV),
		  isNormalRequired(GL_TRUE),
		  isTangentRequired(GL_FALSE),
		  usage(VBO::USAGE_DYNAMIC),
		  borderSize(0.1f) {
}

void FrameMesh::updateAttributes(const Config &cfg) {
    static const Vec3f cubeNormals[] = {
        Vec3f(0.0f, 0.0f, 1.0f), // Front
        Vec3f(0.0f, 0.0f, -1.0f), // Back
        Vec3f(0.0f, 1.0f, 0.0f), // Top
        Vec3f(0.0f, -1.0f, 0.0f), // Bottom
        Vec3f(1.0f, 0.0f, 0.0f), // Right
        Vec3f(-1.0f, 0.0f, 0.0f)  // Left
    };
    static const TriangleVertex cubeVertices[] = {
        // Front
        TriangleVertex(Vec3f(-1.0, -1.0, 1.0), 0),
        TriangleVertex(Vec3f(1.0, -1.0, 1.0), 1),
        TriangleVertex(Vec3f(1.0, 1.0, 1.0), 2),
        TriangleVertex(Vec3f(-1.0, 1.0, 1.0), 3),
        // Back
        TriangleVertex(Vec3f(-1.0, -1.0, -1.0), 0),
        TriangleVertex(Vec3f(-1.0, 1.0, -1.0), 1),
        TriangleVertex(Vec3f(1.0, 1.0, -1.0), 2),
        TriangleVertex(Vec3f(1.0, -1.0, -1.0), 3),
        // Top
        TriangleVertex(Vec3f(-1.0, 1.0, -1.0), 0),
        TriangleVertex(Vec3f(-1.0, 1.0, 1.0), 1),
        TriangleVertex(Vec3f(1.0, 1.0, 1.0), 2),
        TriangleVertex(Vec3f(1.0, 1.0, -1.0), 3),
        // Bottom
        TriangleVertex(Vec3f(-1.0, -1.0, -1.0), 0),
        TriangleVertex(Vec3f(1.0, -1.0, -1.0), 1),
        TriangleVertex(Vec3f(1.0, -1.0, 1.0), 2),
        TriangleVertex(Vec3f(-1.0, -1.0, 1.0), 3),
        // Right
        TriangleVertex(Vec3f(1.0, -1.0, -1.0), 0),
        TriangleVertex(Vec3f(1.0, 1.0, -1.0), 1),
        TriangleVertex(Vec3f(1.0, 1.0, 1.0), 2),
        TriangleVertex(Vec3f(1.0, -1.0, 1.0), 3),
        // Left
        TriangleVertex(Vec3f(-1.0, -1.0, -1.0), 0),
        TriangleVertex(Vec3f(-1.0, -1.0, 1.0), 1),
        TriangleVertex(Vec3f(-1.0, 1.0, 1.0), 2),
        TriangleVertex(Vec3f(-1.0, 1.0, -1.0), 3)
    };

    const GLuint numBoxes = 4;
    const GLuint numFaces = 6;
    // -4 because we can skip left/right face of two border boxes
    GLuint numVertices = pow(4.0, cfg.levelOfDetail) * (numBoxes * numFaces - 4) * 2 * 3;

    pos_->setVertexData(numVertices);
    if (cfg.isNormalRequired) {
        nor_->setVertexData(numVertices);
    }
    if (cfg.isTangentRequired) {
        tan_->setVertexData(numVertices);
    }

    TexcoMode texcoMode = cfg.texcoMode;
    if (cfg.isTangentRequired && cfg.texcoMode == TEXCO_MODE_NONE) {
        texcoMode = TEXCO_MODE_UV;
    }
    if (texcoMode == TEXCO_MODE_UV) {
        texco_ = ref_ptr<ShaderInput2f>::alloc("texco0");
        texco_->setVertexData(numVertices);
    }

    // Define the initial box scale
    Vec3f initialBoxScale(0.5f*cfg.posScale.x, 0.5f*cfg.posScale.y, 0.5f*cfg.borderSize);

    // Define the transformations for the four boxes
    std::vector<Mat4f> transformations;

    // Apply user-supplied rotation
    Mat4f userRotation = Mat4f::rotationMatrix(cfg.rotation.x, cfg.rotation.y, cfg.rotation.z);

    // First box: translate in z direction by 0.5 * cfg.posScale.z
    transformations.push_back(
    	userRotation *
    	Mat4f::translationMatrix_transposed(Vec3f(0.0f, 0.0f, 0.5f*cfg.posScale.z)));

    // Second box: translate in z direction by -0.5 * cfg.posScale.z
    transformations.push_back(
    	userRotation *
    	Mat4f::translationMatrix_transposed(Vec3f(0.0f, 0.0f, -0.5f*cfg.posScale.z)));

    // Third box: rotate by 90 degrees around y-axis, scale, and translate in x direction by 0.5 * cfg.posScale.x
    Mat4f transform3 =
    	userRotation *
    	Mat4f::rotationMatrix(0.0, M_PI_2, 0.0) *
    	Mat4f::scaleMatrix(Vec3f(cfg.posScale.z / (cfg.posScale.x + cfg.borderSize), 1.0f, 1.0f)) *
    	Mat4f::translationMatrix_transposed(Vec3f(0.0, 0.0f, 0.5f*cfg.posScale.x - 0.5f*cfg.borderSize));
    transformations.push_back(transform3);

    // Fourth box: rotate by 90 degrees around y-axis, scale, and translate in x direction by -0.5 * cfg.posScale.x
    Mat4f transform4 =
    	userRotation *
    	Mat4f::rotationMatrix(0.0, M_PI_2, 0.0) *
    	Mat4f::scaleMatrix(Vec3f(cfg.posScale.z / (cfg.posScale.x + cfg.borderSize), 1.0f, 1.0f)) *
    	Mat4f::translationMatrix_transposed(Vec3f(0.0f, 0.0f, -0.5f*cfg.posScale.x + 0.5f*cfg.borderSize));
    transformations.push_back(transform4);

    GLuint vertexBase = 0;
    for (GLuint boxIndex = 0; boxIndex < numBoxes; ++boxIndex) {
        for (GLuint sideIndex = 0; sideIndex < numFaces; ++sideIndex) {
        	if (boxIndex == 2 || boxIndex == 3) {
				if (sideIndex == 4 || sideIndex == 5) {
					continue;
				}
			}
            const Vec3f &normal = cubeNormals[sideIndex];
            auto *level0 = (TriangleVertex *) cubeVertices;
            level0 += sideIndex * 4;

            // Tessellate cube face
            std::vector<TriangleFace> facesLevel0(2);
            facesLevel0[0] = TriangleFace(level0[0], level0[1], level0[3]);
            facesLevel0[1] = TriangleFace(level0[1], level0[2], level0[3]);
            auto faces = tessellate(cfg.levelOfDetail, facesLevel0);

            for (GLuint faceIndex = 0; faceIndex < faces.size(); ++faceIndex) {
                GLuint vertexIndex = faceIndex * 3 + vertexBase;
                TriangleFace &face = faces[faceIndex];
                auto *f = (TriangleVertex *) &face;

                for (GLuint i = 0; i < 3; ++i) {
                    auto transformedVertex = (transformations[boxIndex] * (initialBoxScale * f[i].p));
                    pos_->setVertex(vertexIndex + i, transformedVertex.xyz_());
                }
                if (cfg.isNormalRequired) {
                	auto nor = transformations[boxIndex].rotateVector(normal);
                    for (GLuint i = 0; i < 3; ++i) nor_->setVertex(vertexIndex + i, nor);
                }

                if (texcoMode == TEXCO_MODE_UV) {
                    auto *texco = (Vec2f *) texco_->clientData();

                    for (GLuint i = 0; i < 3; ++i) {
                        // Directly assign UV coordinates based on vertex positions
                        Vec3f pos = f[i].p;
                        // Normalize the vertex position to [0, 1]
                        Vec2f uv;
                        switch (sideIndex) {
                            case 0: // Front face
                            case 1: // Back face
                                uv = Vec2f((pos.x + 1.0f) * 0.5f, (pos.y + 1.0f) * 0.5f);
                                break;
                            case 2: // Top face
                            case 3: // Bottom face
                                uv = Vec2f((pos.x + 1.0f) * 0.5f, (pos.z + 1.0f) * 0.5f);
                                break;
                            case 4: // Right face
                            case 5: // Left face
                                uv = Vec2f((pos.z + 1.0f) * 0.5f, (pos.y + 1.0f) * 0.5f);
                                break;
                            default:
                                uv = Vec2f(0.0f);
                        }
                        texco[vertexIndex + i] = uv * cfg.texcoScale;
                    }
                }

                if (cfg.isTangentRequired) {
                    Vec3f *vertices = ((Vec3f *) pos_->clientDataPtr()) + vertexIndex;
                    Vec2f *texcos = ((Vec2f *) texco_->clientDataPtr()) + vertexIndex;
                    Vec4f tangent = calculateTangent(vertices, texcos, normal);
                    for (GLuint i = 0; i < 3; ++i) tan_->setVertex(vertexIndex + i, tangent);
                }
            }
            vertexBase += faces.size() * 3;
        }
    }

    begin(ShaderInputContainer::INTERLEAVED);
    setInput(pos_);
    if (cfg.isNormalRequired)
        setInput(nor_);
    if (cfg.texcoMode != TEXCO_MODE_NONE)
        setInput(texco_);
    if (cfg.isTangentRequired)
        setInput(tan_);
    end();

    minPosition_ = -cfg.posScale;
    maxPosition_ = cfg.posScale;
}
