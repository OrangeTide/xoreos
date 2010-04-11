/* eos - A reimplementation of BioWare's Aurora engine
 * Copyright (c) 2010 Sven Hesse (DrMcCoy), Matthew Hoops (clone2727)
 *
 * The Infinity, Aurora, Odyssey and Eclipse engines, Copyright (c) BioWare corp.
 * The Electron engine, Copyright (c) Obsidian Entertainment and BioWare corp.
 *
 * This file is part of eos and is distributed under the terms of
 * the GNU General Public Licence. See COPYING for more informations.
 */

/** @file graphics/renderable.h
 *  An object that can be displayed by the graphics manager.
 */

#ifndef GRAPHICS_RENDERABLE_H
#define GRAPHICS_RENDERABLE_H

#include "graphics/graphics.h"

namespace Graphics {

class Renderable {
public:
	Renderable();
	virtual ~Renderable();

	/** Signal the object that it needs to reload its textures. */
	virtual void reloadTextures() = 0;

	/** Render the object. */
	virtual void render() = 0;

	/** Notify the object that it has been kicked out of the render queue. */
	void kickedOutOfRenderQueue();

protected:
	volatile bool _justAddedToQueue; ///< Was the object just added to the queue?

	/** Add the object to the render queue. */
	void addToRenderQueue();
	/** Remove the object from the render queue. */
	void removeFromRenderQueue();

private:
	bool _inRenderQueue; ///< Is the object in the render queue?

	/** A reference to the object's position in the render queue. */
	GraphicsManager::RenderQueueRef _renderQueueRef;
};

} // End of namespace Graphics

#endif // GRAPHICS_RENDERABLE_H
