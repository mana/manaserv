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
 *
 *  $Id$
 */

#include "state.h"
#include <iostream>
#include "messageout.h"

namespace tmwserv
{

void State::update(ConnectionHandler &connectionHandler)
{
    // update game state (update AI, etc.)
    for (std::map<std::string, Beings>::iterator i = beings.begin();
         i != beings.end();
         i++) {
        for (Beings::iterator b = i->second.begin();
             b != i->second.end();
             b++) {
            b->get()->update();
        }
    }

    // notify clients about changes in the game world (only on their maps)
    // NOTE: This isn't finished ;)
    for (std::map<std::string,  Beings>::iterator i = beings.begin();
         i != beings.end();
         i++) {
        //
        for (Beings::iterator b = i->second.begin();
             b != i->second.end();
             b++) {
            // send info about other players
            for (Beings::iterator b2 = i->second.begin();
                 b2 != i->second.end();
                 b2++) {
                if (b != b2) {
                    MessageOut msg;
                    msg.writeShort(SMSG_NEW_OBJECT); // of course this wont be send _all_ the time ;)
                    msg.writeLong(OBJECT_PLAYER);    // type
                    msg.writeLong((int)b2->get());   // id
                    msg.writeLong(b2->get()->getX());// x
                    msg.writeLong(b2->get()->getY());// y

                    connectionHandler.sendTo(b->get(), msg);
                }
            }
        }
    }

}

} // namespace tmwserv
