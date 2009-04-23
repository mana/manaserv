/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana World is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana World; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _TMWSERV_EFFECT_H
#define _TMWSERV_EFFECT_H

#include "game-server/actor.hpp"

class Effect : public Actor
{
    public:
        Effect(int id)
          : Actor(OBJECT_EFFECT)
          , mEffectId(id)
          , mHasBeenShown(false)
        {}

        int getEffectId() const
        { return mEffectId; }

        /**
         * Removes effect after it has been shown.
         */
        virtual void update();

        /**
         * Called when the object has been shown to a player in the state loop.
         */
        void show()
        { mHasBeenShown = true; }

    private:
        int mEffectId;
        bool mHasBeenShown;
};


namespace Effects
{
    /**
     * Convenience method to show an effect.
     */
    void show(int id, MapComposite *map, const Point &pos);

    // TODO: get this in sync with effects.xml
    enum {
       FIRE_BURST        = 15
    };
}

#endif
