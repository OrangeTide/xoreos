/* eos - A reimplementation of BioWare's Aurora engine
 * Copyright (c) 2010-2011 Sven Hesse (DrMcCoy), Matthew Hoops (clone2727)
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 *
 * This file is part of eos and is distributed under the terms of
 * the GNU General Public Licence. See COPYING for more informations.
 */

/** @file aurora/pefile.cpp
 *  A portable executable archive.
 */

#include "common/error.h"
#include "common/ustring.h"
#include "common/file.h"
#include "common/pe_exe.h"
#include "common/filepath.h"

#include "aurora/pefile.h"
#include "aurora/util.h"

namespace Aurora {

PEFile::PEFile(const Common::UString &fileName, const std::vector<Common::UString> &remap) :
	_peFile(0) {

	Common::File *file = new Common::File();
	if (!file->open(fileName)) {
		delete file;
		throw Common::Exception("Could not open exe");
	}

	_peFile = new Common::PEResources();

	if (!_peFile->loadFromEXE(file)) {
		delete file;
		delete _peFile;
		throw Common::Exception("Could not parse exe");
	}

	load(remap);
}

PEFile::~PEFile() {
	delete _peFile;
}

void PEFile::clear() {
	_resources.clear();
}

const Archive::ResourceList &PEFile::getResources() const {
	return _resources;
}

Common::SeekableReadStream *PEFile::getResource(uint32 index) const {
	// Convert from the PE cursor group/cursor format to the standalone
	// cursor format.

	Common::MemoryWriteStreamDynamic out;
	Common::SeekableReadStream *cursorGroup = _peFile->getResource(Common::kPEGroupCursor, index);

	if (!cursorGroup)
		return 0;

	// Cursor Group Header
	out.writeUint16LE(cursorGroup->readUint16LE());
	out.writeUint16LE(cursorGroup->readUint16LE());
	uint16 cursorCount = cursorGroup->readUint16LE();
	out.writeUint16LE(cursorCount);

	std::vector<Common::SeekableReadStream *> cursorStreams;
	cursorStreams.resize(cursorCount);

	uint32 startOffset = 6 + cursorCount * 16;

	for (uint16 i = 0; i < cursorCount; i++) {
		out.writeByte(cursorGroup->readUint16LE());     // width
		out.writeByte(cursorGroup->readUint16LE() / 2); // height
		cursorGroup->readUint16LE();                    // planes
		out.writeByte(cursorGroup->readUint16LE());     // bits per pixel
		out.writeByte(0);                               // reserved

		cursorGroup->readUint32LE();                    // data size
		uint16 id = cursorGroup->readUint16LE();

		Common::SeekableReadStream *cursor = _peFile->getResource(Common::kPECursor, id);
		if (!cursor) {
			warning("Could not get cursor resource %d", id);
			return 0;
		}

		out.writeUint16LE(cursor->readUint16LE());      // hotspot X
		out.writeUint16LE(cursor->readUint16LE());      // hotspot Y
		out.writeUint32LE(cursor->size() - 4);          // size
		out.writeUint32LE(startOffset);                 // offset
		startOffset += cursor->size() - 4;

		cursorStreams[i] = cursor;
	}

	for (uint32 i = 0; i < cursorStreams.size(); i++) {
		byte *data = new byte[cursorStreams[i]->size() - 4];
		cursorStreams[i]->read(data, cursorStreams[i]->size() - 4);
		out.write(data, cursorStreams[i]->size() - 4);
		delete cursorStreams[i];
	}

	return new Common::MemoryReadStream(out.getData(), out.size());
}

void PEFile::load(const std::vector<Common::UString> &remap) {
	std::vector<Common::PEResourceID> cursorList = _peFile->getNameList(Common::kPEGroupCursor);

	for (std::vector<Common::PEResourceID>::const_iterator it = cursorList.begin(); it != cursorList.end(); it++) {
		Resource res;

		if (it->getID() == 0xFFFFFFFF)
			throw Common::Exception("Found non-integer cursor group");

		uint32 id = it->getID() - 1;
		if (id >= remap.size())
			throw Common::Exception("Missing name for cursor %d", id);

		res.name  = remap[id];
		res.type  = kFileTypeCUR;
		res.index = id + 1;

		_resources.push_back(res);
	}
}

} // End of namespace Aurora