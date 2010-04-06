/* eos - A reimplementation of BioWare's Aurora engine
 * Copyright (c) 2010 Sven Hesse (DrMcCoy), Matthew Hoops (clone2727)
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 *
 * This file is part of eos and is distributed under the terms of
 * the GNU General Public Licence. See COPYING for more informations.
 */

/** @file engines/nwn/nwn.cpp
 *  Engine class handling Neverwinter Nights
 */

#include "engines/nwn/nwn.h"

#include "common/util.h"
#include "common/filelist.h"

#include "sound/sound.h"

#include "aurora/resman.h"

namespace NWN {

const NWNEngineProbe kNWNEngineProbe;

Aurora::GameID NWNEngineProbe::getGameID() const {
	return Aurora::kGameIDNWN;
}

bool NWNEngineProbe::probe(const std::string &directory, const Common::FileList &rootFiles) const {
	// Don't accidentally trigger on NWN2
	if (rootFiles.contains(".*/nwn2.ini", true))
		return false;
	if (rootFiles.contains(".*/nwn2main.exe", true))
		return false;

	// If either the ini file or a binary is found, this should be a valid path

	if (rootFiles.contains(".*/nwn.ini", true))
		return true;

	if (rootFiles.contains(".*/(nw|nwn)main.exe", true))
		return true;

	if (rootFiles.contains(".*/(nw|nwn)main", true))
		return true;

	return false;
}

Engines::Engine *NWNEngineProbe::createEngine() const {
	return new NWNEngine;
}


NWNEngine::NWNEngine() {
}

NWNEngine::~NWNEngine() {
}

bool NWNEngine::run(const std::string &directory) {
	_baseDirectory = directory;

	if (!init())
		return false;

	warning("Successfully initialized the engine");

	Common::SeekableReadStream *wav = ResMan.getSound("as_pl_evanglstm1");
	if (wav) {
		warning("Found a wav. Trying to play it. Turn up your speakers");
		SoundMan.playSoundFile(wav);
	}

	return true;
}

bool NWNEngine::init() {
	if (!ResMan.registerDataBaseDir(_baseDirectory))
		return false;

	warning("Loading main KEY");
	if (!indexMandatoryKEY(".*/chitin.key"))
		return false;

	warning("Loading expansions and patch KEYs");

	// Base game patch
	if (!indexOptionalKEY(".*/patch.key"))
		return false;

	// Expansion 1: Shadows of Undrentide (SoU)
	if (!indexOptionalKEY(".*/xp1.key"))
		return false;
	if (!indexOptionalKEY(".*/xp1patch.key"))
		return false;

	// Expansion 2: Hordes of the Underdark (HotU)
	if (!indexOptionalKEY(".*/xp2.key"))
		return false;
	if (!indexOptionalKEY(".*/xp2patch.key"))
		return false;

	// Expansion 3: Kingmaker (resources also included in the final 1.69 patch)
	if (!indexOptionalKEY(".*/xp3.key"))
		return false;
	if (!indexOptionalKEY(".*/xp3patch.key"))
		return false;

	warning("Loading secondary resources");
	if (!ResMan.loadSecondaryResources())
		return false;

	return true;
}

} // End of namespace NWN
