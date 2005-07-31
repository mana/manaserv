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

    // notify clients about changes in the game world
    // NOTE: This isn't finished ;)
    for (std::map<std::string,  Beings>::iterator i = beings.begin();
         i != beings.end();
         i++) {
        //
        for (std::vector<BeingPtr>::iterator b = i->second.begin();
             b != i->second.end();
             b++) {
            // send info about other players
            for (std::vector<BeingPtr>::iterator b2 = i->second.begin();
                 b2 != i->second.end();
                 b2++) {
                if (b != b2) {
                    MessageOut msg;
                    msg.writeShort(SMSG_NEW_OBJECT);
                    msg.writeLong(OBJECT_PLAYER);    // type
                    msg.writeLong((int)b2->get());   // id
                    msg.writeLong(0);                // x
                    msg.writeLong(0);                // y

                    connectionHandler.sendTo(b->get(), msg);
                }
            }
        }
    }

}

} // namespace tmwserv
