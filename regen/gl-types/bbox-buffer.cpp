#include "bbox-buffer.h"

using namespace regen;

struct BoundingBoxBlock {
	Vec4f bboxMin;
	Vec4f bboxMax;
	Vec4i bboxPositiveMin;
	Vec4i bboxPositiveMax;
	Vec4i bboxNegativeMin;
	Vec4i bboxNegativeMax;
	Vec4i bboxPositiveFlags;
	Vec4i bboxNegativeFlags;
};

BBoxBuffer::BBoxBuffer(const std::string &name) :
	SSBO(name, USAGE_STREAM),
	bbox_(Vec3f::zero(), Vec3f::zero())
{
	addBlockInput(createUniform<ShaderInput4f,Vec4f>("bboxMin", Vec4f(0)));
	addBlockInput(createUniform<ShaderInput4f,Vec4f>("bboxMax", Vec4f(0)));
	addBlockInput(createUniform<ShaderInput4i,Vec4i>("bboxPositiveMin", Vec4i(0)));
	addBlockInput(createUniform<ShaderInput4i,Vec4i>("bboxPositiveMax", Vec4i(0)));
	addBlockInput(createUniform<ShaderInput4i,Vec4i>("bboxNegativeMin", Vec4i(0)));
	addBlockInput(createUniform<ShaderInput4i,Vec4i>("bboxNegativeMax", Vec4i(0)));
	addBlockInput(createUniform<ShaderInput4i,Vec4i>("bboxPositiveFlags", Vec4i(0)));
	addBlockInput(createUniform<ShaderInput4i,Vec4i>("bboxNegativeFlags", Vec4i(0)));
	update();

	bboxPBO_ = ref_ptr<PBO>::alloc(USAGE_STREAM);
	bboxPBO_->bindPackBuffer();
	glBufferData(GL_PIXEL_PACK_BUFFER, sizeof(BoundingBoxBlock), nullptr, GL_STREAM_READ);
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
}

bool BBoxBuffer::updateBoundingBox(RenderState *rs) {
	bool hasChanged = false;
    // Read back the bounding box values into the PBO
	rs->copyReadBuffer().push(bufferID());
	rs->copyWriteBuffer().push(bboxPBO_->id());
	glCopyBufferSubData(
			GL_COPY_READ_BUFFER,
			GL_COPY_WRITE_BUFFER,
			blockReference()->address(),
			0,
			2 * sizeof(Vec4f));
	rs->copyWriteBuffer().pop();
	rs->copyReadBuffer().pop();

    // Map the PBO to read the data
	bboxPBO_->bindPackBuffer();
    auto* ptr = (Vec4f*)glMapBuffer(GL_PIXEL_PACK_BUFFER, GL_READ_ONLY);
    if (ptr) {
    	Bounds<Vec3f> newBounds(ptr[0].xyz_(), ptr[1].xyz_());
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
        auto d =
        	(newBounds.min - bbox_.min).length() +
        	(newBounds.max - bbox_.max).length();
		if (d > 0.01f) {
			bbox_.min = newBounds.min;
			bbox_.max = newBounds.max;
			hasChanged = true;
		}
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    return hasChanged;
}
