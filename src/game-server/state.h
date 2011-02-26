/*
 *  The Mana Server
 *  Copyright (C) 2004-2010  The Mana World Development Team
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

#ifndef SERVER_STATE_H
#define SERVER_STATE_H

#include <string>

class MapComposite;
class Thing;
class Actor;
class Character;


namespace GameState
{
    /**
     * Updates game state (contains core server logic).
     */
    void update(int worldTime);

    /**
     * Inserts an thing in the game world.
     * @return false if the insertion failed and the thing is in limbo.
     * @note No update may be in progress.
     */
    bool insert(Thing *)
#   ifdef __GNUC__
    __attribute__((warn_unused_result))
#   endif
    ;

    /**
     * Inserts a thing in the game world. Deletes the thing if the insertion
     * failed.
     * @return false if the insertion failed.
     * @note No update may be in progress. Invalid for characters.
     */
    bool insertOrDelete(Thing *);

    /**
     * Removes a thing from the game world.
     * @note No update may be in progress.
     * @note The thing is not destroyed by this call.
     */
    void remove(Thing *);

    /**
     * Warps a character between places of the game world.
     * @note No update may be in progress.
     * @note The character is destroyed, if needed.
     */
    void warp(Character *, MapComposite *, int x, int y);

    /**
     * Enqueues an insert event.
     * @note The event will be executed at end of update.
     */
    void enqueueInsert(Actor *);

    /**
     * Enqueues a remove event.
     * @note The event will be executed at end of update.
     * @note The thing will be destroyed at that time.
     */
    void enqueueRemove(Actor *);

    /**
     * Enqueues a warp event.
     * @note The event will be executed at end of update.
     */
    void enqueueWarp(Character *, MapComposite *, int x, int y);

    /**
     * Says something to an actor.
     * @note passing NULL as source generates a message from "Server:"
     */
    void sayTo(Actor *destination, Actor *source, const std::string &text);

    /**
     * Says something to everything around an actor.
     */
    void sayAround(Actor *, const std::string &text);

    /**
     * Says something to every player on the server.
     */
    void sayToAll(const std::string &text);

    /**
     * Gets the cached value of a global script variable
     */
    std::string getVariable(const std::string &key);

    /**
     * Changes a global script variable and notifies the database server
     * about the change
     */
    void setVariable (const std::string &key, const std::string &value);

    /**
     * Changes a global variable without notifying the database server
     * about the change
     */
    void setVariableFromDbserver (const std::string &key, const std::string &value);

}

#endif
