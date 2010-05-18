/* eos - A reimplementation of BioWare's Aurora engine
 * Copyright (c) 2010 Sven Hesse (DrMcCoy), Matthew Hoops (clone2727)
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 *
 * This file is part of eos and is distributed under the terms of
 * the GNU General Public Licence. See COPYING for more informations.
 */

/** @file graphics/aurora/model_kotor.cpp
 *  Loading MDL files found in Star Wars: Knights of the Old Republic.
 */

// Disable the "unused variable" warnings while most stuff is still stubbed
#pragma GCC diagnostic ignored "-Wunused-variable"

#include "common/util.h"
#include "common/error.h"
#include "common/maths.h"
#include "common/stream.h"

#include "events/requests.h"

#include "graphics/aurora/model_kotor.h"
#include "graphics/aurora/texture.h"

static const int kNodeFlagHasHeader    = 0x0001;
static const int kNodeFlagHasLight     = 0x0002;
static const int kNodeFlagHasEmitter   = 0x0004;
static const int kNodeFlagHasReference = 0x0010;
static const int kNodeFlagHasMesh      = 0x0020;
static const int kNodeFlagHasSkin      = 0x0040;
static const int kNodeFlagHasAnim      = 0x0080;
static const int kNodeFlagHasDangly    = 0x0100;
static const int kNodeFlagHasAABB      = 0x0200;

static const int kControllerTypePosition             = 8;
static const int kControllerTypeOrientation          = 20;
static const int kControllerTypeScale                = 36;
static const int kControllerTypeColor                = 76;
static const int kControllerTypeRadius               = 88;
static const int kControllerTypeShadowRadius         = 96;
static const int kControllerTypeVerticalDisplacement = 100;
static const int kControllerTypeMultiplier           = 140;
static const int kControllerTypeAlphaEnd             = 80;
static const int kControllerTypeAlphaStart           = 84;
static const int kControllerTypeBirthRate            = 88;
static const int kControllerTypeBounce_Co            = 92;
static const int kControllerTypeColorEnd             = 96;
static const int kControllerTypeColorStart           = 108;
static const int kControllerTypeCombineTime          = 120;
static const int kControllerTypeDrag                 = 124;
static const int kControllerTypeFPS                  = 128;
static const int kControllerTypeFrameEnd             = 132;
static const int kControllerTypeFrameStart           = 136;
static const int kControllerTypeGrav                 = 140;
static const int kControllerTypeLifeExp              = 144;
static const int kControllerTypeMass                 = 148;
static const int kControllerTypeP2P_Bezier2          = 152;
static const int kControllerTypeP2P_Bezier3          = 156;
static const int kControllerTypeParticleRot          = 160;
static const int kControllerTypeRandVel              = 164;
static const int kControllerTypeSizeStart            = 168;
static const int kControllerTypeSizeEnd              = 172;
static const int kControllerTypeSizeStart_Y          = 176;
static const int kControllerTypeSizeEnd_Y            = 180;
static const int kControllerTypeSpread               = 184;
static const int kControllerTypeThreshold            = 188;
static const int kControllerTypeVelocity             = 192;
static const int kControllerTypeXSize                = 196;
static const int kControllerTypeYSize                = 200;
static const int kControllerTypeBlurLength           = 204;
static const int kControllerTypeLightningDelay       = 208;
static const int kControllerTypeLightningRadius      = 212;
static const int kControllerTypeLightningScale       = 216;
static const int kControllerTypeDetonate             = 228;
static const int kControllerTypeAlphaMid             = 464;
static const int kControllerTypeColorMid             = 468;
static const int kControllerTypePercentStart         = 480;
static const int kControllerTypePercentMid           = 481;
static const int kControllerTypePercentEnd           = 482;
static const int kControllerTypeSizeMid              = 484;
static const int kControllerTypeSizeMid_Y            = 488;
static const int kControllerTypeSelfIllumColor       = 100;
static const int kControllerTypeAlpha                = 128;

namespace Graphics {

namespace Aurora {

Model_KotOR::ParserContext::ParserContext(Common::SeekableReadStream &mdlStream,
		Common::SeekableReadStream &mdxStream) : mdl(&mdlStream), mdx(&mdxStream), state(0), node(0), mesh(0) {
}

Model_KotOR::ParserContext::~ParserContext() {
	delete mesh;
	delete node;
	delete state;
}


Model_KotOR::Model_KotOR(Common::SeekableReadStream &mdl,
		Common::SeekableReadStream &mdx, bool kotor2, ModelType type) : Model(type), _kotor2(kotor2) {

	load(mdl, mdx);
	setState();

	_names.clear();

	RequestMan.sync();
}

Model_KotOR::~Model_KotOR() {
}

void Model_KotOR::load(Common::SeekableReadStream &mdl, Common::SeekableReadStream &mdx) {
	ParserContext ctx(mdl, mdx);

	if (ctx.mdl->readUint32LE() != 0)
		throw Common::Exception("Unsupported KotOR ASCII MDL");

	uint32 sizeModelData = ctx.mdl->readUint32LE();
	uint32 sizeRawData   = ctx.mdl->readUint32LE();

	ctx.offModelData = 12;
	ctx.offRawData   = ctx.offModelData + sizeModelData;

	ctx.mdl->skip(8); // Function pointers

	_name.readASCII(*ctx.mdl, 32);

	uint32 nodeHeadPointer = ctx.mdl->readUint32LE();
	uint32 nodeCount       = ctx.mdl->readUint32LE();

	ctx.mdl->skip(24 + 4); // Unknown + Reference count

	uint8 type = ctx.mdl->readByte();

	ctx.mdl->skip(3 + 2); // Padding + Unknown

	uint8 classification = ctx.mdl->readByte();
	uint8 fogged         = ctx.mdl->readByte();

	ctx.mdl->skip(4); // Unknown

	ctx.mdl->skip(12); // TODO: Animation Header Pointer Array

	ctx.mdl->skip(4); // Parent model pointer

	float boundingMin[3], boundingMax[3];

	boundingMin[0] = ctx.mdl->readIEEEFloatLE();
	boundingMin[1] = ctx.mdl->readIEEEFloatLE();
	boundingMin[2] = ctx.mdl->readIEEEFloatLE();

	boundingMax[0] = ctx.mdl->readIEEEFloatLE();
	boundingMax[1] = ctx.mdl->readIEEEFloatLE();
	boundingMax[2] = ctx.mdl->readIEEEFloatLE();

	float radius = ctx.mdl->readIEEEFloatLE();

	_scale = ctx.mdl->readIEEEFloatLE();

	Common::UString superModelName;

	superModelName.readASCII(*ctx.mdl, 32);

	ctx.mdl->skip(4); // Root node pointer again

	ctx.mdl->skip(12); // Unknown

	uint32 nameStart, nameCount;
	readArray(*ctx.mdl, nameStart, nameCount);

	std::vector<uint32> nameOffset;
	readArrayOffsets(*ctx.mdl, nameStart + ctx.offModelData, nameCount, nameOffset);

	readStrings(*ctx.mdl, nameOffset, ctx.offModelData, _names);

	ctx.state = new State;

	readNode(ctx, nodeHeadPointer + ctx.offModelData, 0);

	_states.insert(std::make_pair(ctx.state->name, ctx.state));
	ctx.state = 0;
}

void Model_KotOR::readNode(ParserContext &ctx, uint32 offset, Node *parent) {
	ctx.mdl->seekTo(offset);

	ctx.mesh = new Mesh;
	ctx.node = new Node;

	if (parent) {
		ctx.node->parent = parent;
		parent->children.push_back(ctx.node);
	} else
		ctx.state->nodes.push_back(ctx.node);

	uint16 flags      = ctx.mdl->readUint16LE();
	uint16 superNode  = ctx.mdl->readUint16LE();
	uint16 nodeNumber = ctx.mdl->readUint16LE();

	ctx.mdl->skip(6 + 4); // Unknown + parent pointer

	ctx.node->position   [0] = ctx.mdl->readIEEEFloatLE();
	ctx.node->position   [1] = ctx.mdl->readIEEEFloatLE();
	ctx.node->position   [2] = ctx.mdl->readIEEEFloatLE();
	ctx.node->orientation[3] = Common::rad2deg(acos(ctx.mdl->readIEEEFloatLE()) * 2.0);
	ctx.node->orientation[0] = ctx.mdl->readIEEEFloatLE();
	ctx.node->orientation[1] = ctx.mdl->readIEEEFloatLE();
	ctx.node->orientation[2] = ctx.mdl->readIEEEFloatLE();

	uint32 childrenStart, childrenCount;
	readArray(*ctx.mdl, childrenStart, childrenCount);

	std::vector<uint32> children;
	readArrayOffsets(*ctx.mdl, childrenStart + ctx.offModelData, childrenCount, children);

	uint32 controllerKeyStart, controllerKeyCount;
	readArray(*ctx.mdl, controllerKeyStart, controllerKeyCount);

	uint32 controllerDataStart, controllerDataCount;
	readArray(*ctx.mdl, controllerDataStart, controllerDataCount);

	std::vector<float> controllerData;
	readArrayFloats(*ctx.mdl, controllerDataStart + ctx.offModelData, controllerDataCount, controllerData);

	readNodeControllers(ctx, controllerKeyStart + ctx.offModelData, controllerKeyCount, controllerData);

	if ((flags & 0xFC00) != 0)
		throw Common::Exception("Unknown node flags %04X", flags);

	if (flags & kNodeFlagHasLight) {
		// TODO: Light
		ctx.mdl->skip(0x5C);
	}

	if (flags & kNodeFlagHasEmitter) {
		// TODO: Emitter
		ctx.mdl->skip(0xD8);
	}

	if (flags & kNodeFlagHasReference) {
		// TODO: Reference
		ctx.mdl->skip(0x44);
	}

	if (flags & kNodeFlagHasMesh) {
		readMesh(ctx);
	}

	if (flags & kNodeFlagHasSkin) {
		// TODO: Skin
		ctx.mdl->skip(0x64);
	}

	if (flags & kNodeFlagHasAnim) {
		// TODO: Anim
		ctx.mdl->skip(0x38);
	}

	if (flags & kNodeFlagHasDangly) {
		// TODO: Dangly
		ctx.mdl->skip(0x18);
	}

	if (flags & kNodeFlagHasAABB) {
		// TODO: AABB
		ctx.mdl->skip(0x4);
	}

	if (nodeNumber < _names.size())
		ctx.node->name = _names[nodeNumber];

	processMesh(*ctx.mesh, *ctx.node);
	delete ctx.mesh;
	ctx.mesh = 0;

	parent = ctx.node;

	_nodes.push_back(ctx.node);
	ctx.node = 0;

	for (std::vector<uint32>::const_iterator child = children.begin(); child != children.end(); ++child)
		readNode(ctx, *child + ctx.offModelData, parent);
}

void Model_KotOR::readMesh(ParserContext &ctx) {
	uint32 P = ctx.mdl->pos();

	ctx.mdl->skip(8); // Function pointers

	uint32 facesStart, facesCount;
	readArray(*ctx.mdl, facesStart, facesCount);

	float boundingMin[3], boundingMax[3];

	boundingMin[0] = ctx.mdl->readIEEEFloatLE();
	boundingMin[1] = ctx.mdl->readIEEEFloatLE();
	boundingMin[2] = ctx.mdl->readIEEEFloatLE();

	boundingMax[0] = ctx.mdl->readIEEEFloatLE();
	boundingMax[1] = ctx.mdl->readIEEEFloatLE();
	boundingMax[2] = ctx.mdl->readIEEEFloatLE();

	float radius = ctx.mdl->readIEEEFloatLE();

	float pointsAverage[3];
	pointsAverage[0] = ctx.mdl->readIEEEFloatLE();
	pointsAverage[1] = ctx.mdl->readIEEEFloatLE();
	pointsAverage[2] = ctx.mdl->readIEEEFloatLE();

	ctx.node->diffuse[0] = ctx.mdl->readIEEEFloatLE();
	ctx.node->diffuse[1] = ctx.mdl->readIEEEFloatLE();
	ctx.node->diffuse[2] = ctx.mdl->readIEEEFloatLE();

	ctx.node->ambient[0] = ctx.mdl->readIEEEFloatLE();
	ctx.node->ambient[1] = ctx.mdl->readIEEEFloatLE();
	ctx.node->ambient[2] = ctx.mdl->readIEEEFloatLE();

	ctx.node->specular[0] = 0;
	ctx.node->specular[1] = 0;
	ctx.node->specular[2] = 0;

	ctx.node->shininess = ctx.mdl->readIEEEFloatLE();

	Common::UString textures[2];
	textures[0].readASCII(*ctx.mdl, 32);
	textures[1].readASCII(*ctx.mdl, 32);

	ctx.mdl->skip(24); // Unknown

	ctx.mdl->skip(12); // Vertex indices counts

	uint32 offOffVerts, offOffVertsCount;
	readArray(*ctx.mdl, offOffVerts, offOffVertsCount);

	if (offOffVertsCount != 1)
		throw Common::Exception("Face offsets offsets count wrong");

	ctx.mdl->skip(12); // Unknown

	ctx.mdl->skip(24 + 16); // Unknown

	uint32 mdxStructSize = ctx.mdl->readUint32LE();

	ctx.mdl->skip(8); // Unknown

	uint32 offNormals = ctx.mdl->readUint32LE();

	ctx.mdl->skip(4); // Unknown

	uint32 offUV = ctx.mdl->readUint32LE();

	ctx.mdl->skip(28); // Unknown

	uint16 vertexCount  = ctx.mdl->readUint16LE();
	uint16 textureCount = ctx.mdl->readUint16LE();

	ctx.mdl->skip(2);

	byte unknownFlag1 = ctx.mdl->readByte();
	ctx.node->shadow  = ctx.mdl->readByte() == 1;
	byte unknownFlag2 = ctx.mdl->readByte();
	ctx.node->render  = ctx.mdl->readByte() == 1;

	ctx.mdl->skip(10);

	if (_kotor2)
		ctx.mdl->skip(8);

	uint32 offNodeData = ctx.mdl->readUint32LE();

	ctx.mdl->skip(4);

	uint32 endPos = ctx.mdl->pos();

	if (textureCount > 1)
		warning("Model_KotOR::readMesh(): textureCount == %d (\"%s\", \"%s\")", textureCount,
				textures[0].c_str(), textures[1].c_str());

	ctx.mesh->texture = textures[0];

	ctx.mesh-> verts.resize(3 * vertexCount);
	ctx.mesh->tverts.resize(3 * vertexCount);
	for (int i = 0; i < vertexCount; i++) {
		ctx.mdx->seekTo(offNodeData + i * mdxStructSize);

		ctx.mesh->verts[3 * i + 0] = ctx.mdx->readIEEEFloatLE();
		ctx.mesh->verts[3 * i + 1] = ctx.mdx->readIEEEFloatLE();
		ctx.mesh->verts[3 * i + 2] = ctx.mdx->readIEEEFloatLE();

		if (offUV != 0xFFFFFFFF) {
			ctx.mdx->seekTo(offNodeData + i * mdxStructSize + offUV);

			ctx.mesh->tverts[3 * i + 0] = ctx.mdx->readIEEEFloatLE();
			ctx.mesh->tverts[3 * i + 1] = ctx.mdx->readIEEEFloatLE();
			ctx.mesh->tverts[3 * i + 2] = 0.0;
		} else {
			ctx.mesh->tverts[3 * i + 0] = 0.0;
			ctx.mesh->tverts[3 * i + 1] = 0.0;
			ctx.mesh->tverts[3 * i + 2] = 0.0;
		}
	}

	ctx.mdl->seekTo(offOffVerts + ctx.offModelData);
	uint32 offVerts = ctx.mdl->readUint32LE();

	ctx.mdl->seekTo(offVerts + ctx.offModelData);

	ctx.mesh->faces.resize(facesCount);
	for (uint32 i = 0; i < facesCount; i++) {
		MeshFace &face = ctx.mesh->faces[i];

		face.tverts[0] = face.verts[0] = ctx.mdl->readUint16LE();
		face.tverts[1] = face.verts[1] = ctx.mdl->readUint16LE();
		face.tverts[2] = face.verts[2] = ctx.mdl->readUint16LE();
	}

	ctx.mdl->seekTo(endPos);
}

void Model_KotOR::readStrings(Common::SeekableReadStream &mdl, const std::vector<uint32> &offsets,
		uint32 offset, std::vector<Common::UString> &strings) {

	uint32 pos = mdl.pos();

	strings.resize(offsets.size());
	for (uint32 i = 0; i < offsets.size(); i++) {
		mdl.seekTo(offsets[i] + offset);
		strings[i].readASCII(mdl);
	}

	mdl.seekTo(pos);
}

void Model_KotOR::readNodeControllers(ParserContext &ctx, uint32 offset, uint32 count, std::vector<float> &data) {
	uint32 pos = ctx.mdl->seekTo(offset);

	// TODO: Implement this properly :P

	for (uint32 i = 0; i < count; i++) {
		uint32 type        = ctx.mdl->readUint32LE();
		uint16 rowCount    = ctx.mdl->readUint16LE();
		uint16 timeIndex   = ctx.mdl->readUint16LE();
		uint16 dataIndex   = ctx.mdl->readUint16LE();
		uint8  columnCount = ctx.mdl->readByte();
		ctx.mdl->skip(1);

		if (columnCount == 0xFFFF)
			throw Common::Exception("TODO: Model_KotOR::readNodeControllers(): columnCount == 0xFFFF");

		/*
		if        (type == kControllerTypePosition) {
			if (columnCount != 3)
				throw Common::Exception("Position controller with %d values", columnCount);

			ctx.node->position[0] = data[dataIndex + 0];
			ctx.node->position[1] = data[dataIndex + 1];
			ctx.node->position[2] = data[dataIndex + 2];

		} else if (type == kControllerTypeOrientation) {
			if (columnCount != 4)
				throw Common::Exception("Orientation controller with %d values", columnCount);

			ctx.node->orientation[0] = data[dataIndex + 0];
			ctx.node->orientation[1] = data[dataIndex + 1];
			ctx.node->orientation[2] = data[dataIndex + 2];
			ctx.node->orientation[3] = data[dataIndex + 3];
		}
		*/

	}

	ctx.mdl->seekTo(pos);
}

} // End of namespace Aurora

} // End of namespace Graphics