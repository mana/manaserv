/*
 *  The Mana Server
 *  Copyright (C) 2004  The Mana World Development Team
 *
 *  This file is part of The Mana Server.
 *
 *  The Mana Server is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  any later version.
 *
 *  The Mana Server is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with The Mana Server.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef EFFECT_H
#define EFFECT_H

#include "game-server/actor.hpp"
#include "game-server/being.hpp"

class Effect : public Actor
{
    public:
        Effect(int id)
          : Actor(OBJECT_EFFECT)
          , mEffectId(id)
          , mHasBeenShown(false)
          , mBeing(NULL)
        {}

        int getEffectId() const
        { return mEffectId; }

        Being *getBeing() const
        { return mBeing; }

        /**
         * Removes effect after it has been shown.
         */
        virtual void update();

        /**
         * Called when the object has been shown to a player in the state loop.
         */
        void show()
        { mHasBeenShown = true; }


        bool setBeing(Being *b)
        {
            if (b)
            {
                setPosition(b->getPosition());
                mBeing = b;
                return true;
            } else {
                return false;
            }
        }

    private:
        int mEffectId;
        bool mHasBeenShown;
        Being *mBeing;
};


namespace Effects
{
    /**
     * Convenience methods to show an effect.
     */
    void show(int id, MapComposite *map, const Point &pos);
    void show(int id, MapComposite *map, Being *b);

    // TODO: get this in sync with effects.xml
    enum {
       FIRE_BURST        = 15
    };
}

#endif
