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

#include "game-server/state.h"

#include "common/configuration.h"
#include "game-server/accountconnection.h"
#include "game-server/effect.h"
#include "game-server/combatcomponent.h"
#include "game-server/gamehandler.h"
#include "game-server/inventory.h"
#include "game-server/item.h"
#include "game-server/itemmanager.h"
#include "game-server/map.h"
#include "game-server/mapcomposite.h"
#include "game-server/mapmanager.h"
#include "game-server/monster.h"
#include "game-server/npc.h"
#include "game-server/trade.h"
#include "net/messageout.h"
#include "scripting/script.h"
#include "scripting/scriptmanager.h"
#include "utils/logger.h"
#include "utils/speedconv.h"

#include <cassert>

enum
{
    EVENT_REMOVE = 0,
    EVENT_INSERT,
    EVENT_WARP
};

/**
 * Event expected to happen at next update.
 */
struct DelayedEvent
{
    unsigned short type, x, y;
    MapComposite *map;
};

typedef std::map< Actor *, DelayedEvent > DelayedEvents;

/**
 * The current world time in ticks since server start.
 */
static int currentTick;

/**
 * List of delayed events.
 */
static DelayedEvents delayedEvents;

/**
 * Cached persistent script variables
 */
static std::map< std::string, std::string > mScriptVariables;

/**
 * Sets message fields describing character look.
 */
static void serializeLooks(Character *ch, MessageOut &msg)
{
    const EquipData &equipData = ch->getPossessions().getEquipment();

    // We'll use a set to check whether we already sent the update for the given
    // item instance.
    std::set<unsigned> itemInstances;

    // The map storing the info about the look changes to send
    //{ slot type id, item id }
    std::map <unsigned, unsigned> lookChanges;

    // Note that we can send several updates on the same slot type as different
    // items may have been equipped.
    for (EquipData::const_iterator it = equipData.begin(),
         it_end = equipData.end(); it != it_end; ++it)
    {
        if (!itemManager->isEquipSlotVisible(it->first))
            continue;

        if (!it->second.itemInstance
            || itemInstances.insert(it->second.itemInstance).second)
        {
            // When the insertion succeeds, its the first time
            // we encounter the item, so we can send the look change.
            // We also send empty slots for unequipment handling.
            lookChanges.insert(
                std::make_pair<unsigned, unsigned>(it->first,
                                                   it->second.itemId));
        }
    }

    if (lookChanges.size() > 0)
    {
        // Number of look changes to send
        msg.writeInt8(lookChanges.size());

        for (std::map<unsigned, unsigned>::const_iterator it2 =
             lookChanges.begin(), it2_end = lookChanges.end();
             it2 != it2_end; ++it2)
        {
            msg.writeInt8(it2->first);
            msg.writeInt16(it2->second);
        }
    }
}

/**
 * Informs a player of what happened around the character.
 */
static void informPlayer(MapComposite *map, Character *p)
{
    MessageOut moveMsg(GPMSG_BEINGS_MOVE);
    MessageOut damageMsg(GPMSG_BEINGS_DAMAGE);
    const Point &pold = p->getOldPosition(), ppos = p->getPosition();
    int pid = p->getPublicID(), pflags = p->getUpdateFlags();
    int visualRange = Configuration::getValue("game_visualRange", 448);

    // Inform client about activities of other beings near its character
    for (BeingIterator it(map->getAroundBeingIterator(p, visualRange));
         it; ++it)
    {
        Being *o = *it;

        const Point &oold = o->getOldPosition(), opos = o->getPosition();
        int otype = o->getType();
        int oid = o->getPublicID(), oflags = o->getUpdateFlags();
        int flags = 0;

        // Check if the character p and the moving object o are around.
        bool wereInRange = pold.inRangeOf(oold, visualRange) &&
                           !((pflags | oflags) & UPDATEFLAG_NEW_ON_MAP);
        bool willBeInRange = ppos.inRangeOf(opos, visualRange);

        if (!wereInRange && !willBeInRange)
        {
            // Nothing to report: o and p are far away from each other.
            continue;
        }


        if (wereInRange && willBeInRange)
        {
            // Send attack messages.
            if ((oflags & UPDATEFLAG_ATTACK) && oid != pid)
            {
                MessageOut AttackMsg(GPMSG_BEING_ATTACK);
                AttackMsg.writeInt16(oid);
                AttackMsg.writeInt8(o->getDirection());
                CombatComponent *combatComponent =
                        o->getComponent<CombatComponent>();
                AttackMsg.writeInt8(combatComponent->getAttackId());
                gameHandler->sendTo(p, AttackMsg);
            }

            // Send action change messages.
            if ((oflags & UPDATEFLAG_ACTIONCHANGE))
            {
                MessageOut ActionMsg(GPMSG_BEING_ACTION_CHANGE);
                ActionMsg.writeInt16(oid);
                ActionMsg.writeInt8(static_cast< Being * >(o)->getAction());
                gameHandler->sendTo(p, ActionMsg);
            }

            // Send looks change messages.
            if (oflags & UPDATEFLAG_LOOKSCHANGE)
            {
                MessageOut LooksMsg(GPMSG_BEING_LOOKS_CHANGE);
                LooksMsg.writeInt16(oid);
                Character * c = static_cast<Character * >(o);
                serializeLooks(c, LooksMsg);
                LooksMsg.writeInt16(c->getHairStyle());
                LooksMsg.writeInt16(c->getHairColor());
                LooksMsg.writeInt16(c->getGender());
                gameHandler->sendTo(p, LooksMsg);
            }

            // Send emote messages.
            if (oflags & UPDATEFLAG_EMOTE)
            {
                int emoteId = o->getLastEmote();
                if (emoteId > -1)
                {
                    MessageOut EmoteMsg(GPMSG_BEING_EMOTE);
                    EmoteMsg.writeInt16(oid);
                    EmoteMsg.writeInt16(emoteId);
                    gameHandler->sendTo(p, EmoteMsg);
                }
            }

            // Send direction change messages.
            if (oflags & UPDATEFLAG_DIRCHANGE)
            {
                MessageOut DirMsg(GPMSG_BEING_DIR_CHANGE);
                DirMsg.writeInt16(oid);
                DirMsg.writeInt8(o->getDirection());
                gameHandler->sendTo(p, DirMsg);
            }

            // Send damage messages.
            if (o->canFight())
            {
                CombatComponent *combatComponent =
                        o->getComponent<CombatComponent>();
                const Hits &hits = combatComponent->getHitsTaken();
                for (Hits::const_iterator j = hits.begin(),
                     j_end = hits.end(); j != j_end; ++j)
                {
                    damageMsg.writeInt16(oid);
                    damageMsg.writeInt16(*j);
                }
            }

            if (oold == opos)
            {
                // o does not move, nothing more to report.
                continue;
            }
        }

        if (!willBeInRange)
        {
            // o is no longer visible from p. Send leave message.
            MessageOut leaveMsg(GPMSG_BEING_LEAVE);
            leaveMsg.writeInt16(oid);
            gameHandler->sendTo(p, leaveMsg);
            continue;
        }

        if (!wereInRange)
        {
            // o is now visible by p. Send enter message.
            MessageOut enterMsg(GPMSG_BEING_ENTER);
            enterMsg.writeInt8(otype);
            enterMsg.writeInt16(oid);
            enterMsg.writeInt8(static_cast< Being *>(o)->getAction());
            enterMsg.writeInt16(opos.x);
            enterMsg.writeInt16(opos.y);
            enterMsg.writeInt8(o->getDirection());
            enterMsg.writeInt8(o->getGender());
            switch (otype)
            {
                case OBJECT_CHARACTER:
                {
                    Character *q = static_cast< Character * >(o);
                    enterMsg.writeString(q->getName());
                    enterMsg.writeInt8(q->getHairStyle());
                    enterMsg.writeInt8(q->getHairColor());
                    serializeLooks(q, enterMsg);
                } break;

                case OBJECT_MONSTER:
                {
                    Monster *q = static_cast< Monster * >(o);
                    enterMsg.writeInt16(q->getSpecy()->getId());
                    enterMsg.writeString(q->getName());
                } break;

                case OBJECT_NPC:
                {
                    NpcComponent *npcComponent = o->getComponent<NpcComponent>();
                    enterMsg.writeInt16(npcComponent->getNpcId());
                    enterMsg.writeString(o->getName());
                } break;

                default:
                    assert(false); // TODO
                    break;
            }
            gameHandler->sendTo(p, enterMsg);
        }

        if (opos != oold)
        {
            // Add position check coords every 5 seconds.
            if (currentTick % 50 == 0)
                flags |= MOVING_POSITION;

            flags |= MOVING_DESTINATION;
        }

        // Send move messages.
        moveMsg.writeInt16(oid);
        moveMsg.writeInt8(flags);
        if (flags & MOVING_POSITION)
        {
            moveMsg.writeInt16(oold.x);
            moveMsg.writeInt16(oold.y);
        }

        if (flags & MOVING_DESTINATION)
        {
            moveMsg.writeInt16(opos.x);
            moveMsg.writeInt16(opos.y);
            // We multiply the sent speed (in tiles per second) by ten
            // to get it within a byte with decimal precision.
            // For instance, a value of 4.5 will be sent as 45.
            moveMsg.writeInt8((unsigned short)
                (o->getModifiedAttribute(ATTR_MOVE_SPEED_TPS) * 10));
        }
    }

    // Do not send a packet if nothing happened in p's range.
    if (moveMsg.getLength() > 2)
        gameHandler->sendTo(p, moveMsg);

    if (damageMsg.getLength() > 2)
        gameHandler->sendTo(p, damageMsg);

    // Inform client about status change.
    p->sendStatus();

    // Inform client about health change of party members
    for (CharacterIterator i(map->getWholeMapIterator()); i; ++i)
    {
        Character *c = *i;

        // Make sure its not the same character
        if (c == p)
            continue;

        // make sure they are in the same party
        if (c->getParty() == p->getParty())
        {
            int cflags = c->getUpdateFlags();
            if (cflags & UPDATEFLAG_HEALTHCHANGE)
            {
                MessageOut healthMsg(GPMSG_BEING_HEALTH_CHANGE);
                healthMsg.writeInt16(c->getPublicID());
                healthMsg.writeInt16(c->getModifiedAttribute(ATTR_HP));
                healthMsg.writeInt16(c->getModifiedAttribute(ATTR_MAX_HP));
                gameHandler->sendTo(p, healthMsg);
            }
        }
    }

    // Inform client about items on the ground around its character
    MessageOut itemMsg(GPMSG_ITEMS);
    for (FixedActorIterator it(map->getAroundBeingIterator(p, visualRange));
         it; ++it)
    {
        Actor *o = *it;

        assert(o->getType() == OBJECT_ITEM ||
               o->getType() == OBJECT_EFFECT);

        Point opos = o->getPosition();
        int oflags = o->getUpdateFlags();
        bool willBeInRange = ppos.inRangeOf(opos, visualRange);
        bool wereInRange = pold.inRangeOf(opos, visualRange) &&
                           !((pflags | oflags) & UPDATEFLAG_NEW_ON_MAP);

        if (willBeInRange ^ wereInRange)
        {
            switch (o->getType())
            {
                case OBJECT_ITEM:
                {
                    ItemComponent *item = o->getComponent<ItemComponent>();
                    ItemClass *itemClass = item->getItemClass();

                    if (oflags & UPDATEFLAG_NEW_ON_MAP)
                    {
                        /* Send a specific message to the client when an item appears
                           out of nowhere, so that a sound/animation can be performed. */
                        MessageOut appearMsg(GPMSG_ITEM_APPEAR);
                        appearMsg.writeInt16(itemClass->getDatabaseID());
                        appearMsg.writeInt16(opos.x);
                        appearMsg.writeInt16(opos.y);
                        gameHandler->sendTo(p, appearMsg);
                    }
                    else
                    {
                        itemMsg.writeInt16(willBeInRange ? itemClass->getDatabaseID() : 0);
                        itemMsg.writeInt16(opos.x);
                        itemMsg.writeInt16(opos.y);
                    }
                }
                break;
                case OBJECT_EFFECT:
                {
                    EffectComponent *e = o->getComponent<EffectComponent>();
                    e->setShown();
                    // Don't show old effects
                    if (!(oflags & UPDATEFLAG_NEW_ON_MAP))
                        break;

                    if (Being *b = e->getBeing())
                    {
                        MessageOut effectMsg(GPMSG_CREATE_EFFECT_BEING);
                        effectMsg.writeInt16(e->getEffectId());
                        effectMsg.writeInt16(b->getPublicID());
                        gameHandler->sendTo(p, effectMsg);
                    } else {
                        MessageOut effectMsg(GPMSG_CREATE_EFFECT_POS);
                        effectMsg.writeInt16(e->getEffectId());
                        effectMsg.writeInt16(opos.x);
                        effectMsg.writeInt16(opos.y);
                        gameHandler->sendTo(p, effectMsg);
                    }
                }
                break;
                default: break;
            } // Switch
        }
    }

    // Do not send a packet if nothing happened in p's range.
    if (itemMsg.getLength() > 2)
        gameHandler->sendTo(p, itemMsg);
}

#ifndef NDEBUG
static bool dbgLockObjects;
#endif

void GameState::update(int tick)
{
    currentTick = tick;

#ifndef NDEBUG
    dbgLockObjects = true;
#endif

    ScriptManager::currentState()->update();

    // Update game state (update AI, etc.)
    const MapManager::Maps &maps = MapManager::getMaps();
    for (MapManager::Maps::const_iterator m = maps.begin(),
         m_end = maps.end(); m != m_end; ++m)
    {
        MapComposite *map = m->second;
        if (!map->isActive())
            continue;

        map->update();

        for (CharacterIterator p(map->getWholeMapIterator()); p; ++p)
        {
            informPlayer(map, *p);
        }

        for (ActorIterator it(map->getWholeMapIterator()); it; ++it)
        {
            Actor *a = *it;
            a->clearUpdateFlags();
            if (a->canFight())
            {
                a->getComponent<CombatComponent>()->clearHitsTaken();
            }
        }
    }

#   ifndef NDEBUG
    dbgLockObjects = false;
#   endif

    // Take care of events that were delayed because of their side effects.
    for (DelayedEvents::iterator it = delayedEvents.begin(),
         it_end = delayedEvents.end(); it != it_end; ++it)
    {
        const DelayedEvent &e = it->second;
        Actor *o = it->first;
        switch (e.type)
        {
            case EVENT_REMOVE:
                remove(o);
                if (o->getType() == OBJECT_CHARACTER)
                {
                    Character *ch = static_cast< Character * >(o);
                    ch->disconnected();
                    gameHandler->kill(ch);
                }
                delete o;
                break;

            case EVENT_INSERT:
                insertOrDelete(o);
                break;

            case EVENT_WARP:
                assert(o->getType() == OBJECT_CHARACTER);
                warp(static_cast< Character * >(o), e.map, e.x, e.y);
                break;
        }
    }
    delayedEvents.clear();
}

bool GameState::insert(Entity *ptr)
{
    assert(!dbgLockObjects);
    MapComposite *map = ptr->getMap();
    assert(map && map->isActive());

    /* Non-visible objects have neither position nor public ID, so their
       insertion cannot fail. Take care of them first. */
    if (!ptr->isVisible())
    {
        map->insert(ptr);
        ptr->signal_inserted.emit(ptr);
        return true;
    }

    // Check that coordinates are actually valid.
    Actor *obj = static_cast< Actor * >(ptr);
    Map *mp = map->getMap();
    Point pos = obj->getPosition();
    if ((int)pos.x / mp->getTileWidth() >= mp->getWidth() ||
        (int)pos.y / mp->getTileHeight() >= mp->getHeight())
    {
        LOG_ERROR("Tried to insert an actor at position " << pos.x << ','
                  << pos.y << " outside map " << map->getID() << '.');
        // Set an arbitrary small position.
        pos = Point(100, 100);
        obj->setPosition(pos);
    }

    if (!map->insert(obj))
    {
        // The map is overloaded, no room to add a new actor
        LOG_ERROR("Too many actors on map " << map->getID() << '.');
        return false;
    }

    obj->signal_inserted.emit(obj);

    // DEBUG INFO
    switch (obj->getType())
    {
        case OBJECT_ITEM:
            LOG_DEBUG("Item inserted: "
                   << obj->getComponent<ItemComponent>()->getItemClass()->getDatabaseID());
            break;

        case OBJECT_NPC:
            LOG_DEBUG("NPC inserted: " << obj->getComponent<NpcComponent>()->getNpcId());
            break;

        case OBJECT_CHARACTER:
            LOG_DEBUG("Player inserted: "
                      << static_cast<Being*>(obj)->getName());
            break;

        case OBJECT_EFFECT:
            LOG_DEBUG("Effect inserted: "
                      << obj->getComponent<EffectComponent>()->getEffectId());
            break;

        case OBJECT_MONSTER:
            LOG_DEBUG("Monster inserted: "
                      << static_cast<Monster*>(obj)->getSpecy()->getId());
            break;

        case OBJECT_ACTOR:
        case OBJECT_OTHER:
        default:
            LOG_DEBUG("Entity inserted: " << obj->getType());
            break;
    }

    obj->raiseUpdateFlags(UPDATEFLAG_NEW_ON_MAP);
    if (obj->getType() != OBJECT_CHARACTER)
        return true;

    /* Since the player does not know yet where in the world its character is,
       we send a map-change message, even if it is the first time it
       connects to this server. */
    MessageOut mapChangeMessage(GPMSG_PLAYER_MAP_CHANGE);
    mapChangeMessage.writeString(map->getName());
    mapChangeMessage.writeInt16(pos.x);
    mapChangeMessage.writeInt16(pos.y);
    gameHandler->sendTo(static_cast< Character * >(obj), mapChangeMessage);

    // update the online state of the character
    accountHandler->updateOnlineStatus(
        static_cast< Character * >(obj)->getDatabaseID(), true);

    return true;
}

int GameState::getCurrentTick()
{
    return currentTick;
}

bool GameState::insertOrDelete(Entity *ptr)
{
    if (insert(ptr)) return true;
    delete ptr;
    return false;
}

void GameState::remove(Entity *ptr)
{
    assert(!dbgLockObjects);
    MapComposite *map = ptr->getMap();
    int visualRange = Configuration::getValue("game_visualRange", 448);

    ptr->signal_removed.emit(ptr);

    // DEBUG INFO
    switch (ptr->getType())
    {
        case OBJECT_ITEM:
            LOG_DEBUG("Item removed: "
                   << ptr->getComponent<ItemComponent>()->getItemClass()->getDatabaseID());
            break;

        case OBJECT_NPC:
            LOG_DEBUG("NPC removed: " << ptr->getComponent<NpcComponent>()->getNpcId());
            break;

        case OBJECT_CHARACTER:
            LOG_DEBUG("Player removed: "
                      << static_cast<Being*>(ptr)->getName());
            break;

        case OBJECT_EFFECT:
            LOG_DEBUG("Effect removed: "
                      << ptr->getComponent<EffectComponent>()->getEffectId());
            break;

        case OBJECT_MONSTER:
            LOG_DEBUG("Monster removed: "
                      << static_cast<Monster*>(ptr)->getSpecy()->getId());
            break;

        case OBJECT_ACTOR:
        case OBJECT_OTHER:
        default:
            LOG_DEBUG("Entity removed: " << ptr->getType());
            break;
    }

    if (ptr->canMove())
    {
        if (ptr->getType() == OBJECT_CHARACTER)
        {
            static_cast< Character * >(ptr)->cancelTransaction();

            // remove characters online status
            accountHandler->updateOnlineStatus(
                static_cast< Character * >(ptr)->getDatabaseID(), false);
        }

        Actor *obj = static_cast< Actor * >(ptr);
        MessageOut msg(GPMSG_BEING_LEAVE);
        msg.writeInt16(obj->getPublicID());
        Point objectPos = obj->getPosition();

        for (CharacterIterator p(map->getAroundActorIterator(obj, visualRange));
             p; ++p)
        {
            if (*p != obj && objectPos.inRangeOf((*p)->getPosition(),
                visualRange))
            {
                gameHandler->sendTo(*p, msg);
            }
        }
    }
    else if (ptr->getType() == OBJECT_ITEM)
    {
        Actor *actor = static_cast<Actor*>(ptr);
        Point pos = actor->getPosition();
        MessageOut msg(GPMSG_ITEMS);
        msg.writeInt16(0);
        msg.writeInt16(pos.x);
        msg.writeInt16(pos.y);

        for (CharacterIterator p(map->getAroundActorIterator(actor, visualRange)); p; ++p)
        {
            if (pos.inRangeOf((*p)->getPosition(), visualRange))
            {
                gameHandler->sendTo(*p, msg);
            }
        }
    }

    map->remove(ptr);
}

void GameState::warp(Character *ptr, MapComposite *map, int x, int y)
{
    remove(ptr);
    ptr->setMap(map);
    ptr->setPosition(Point(x, y));
    ptr->clearDestination();
    /* Force update of persistent data on map change, so that
       characters can respawn at the start of the map after a death or
       a disconnection. */
    accountHandler->sendCharacterData(ptr);

    // If the player has just left, The character pointer is also about
    // to be deleted. So we don't have to do anything else.
    if (!ptr->isConnected())
        return;

    if (map->isActive())
    {
        if (!insert(ptr))
        {
            ptr->disconnected();
            gameHandler->kill(ptr);
            delete ptr;
        }
    }
    else
    {
        MessageOut msg(GAMSG_REDIRECT);
        msg.writeInt32(ptr->getDatabaseID());
        accountHandler->send(msg);
        gameHandler->prepareServerChange(ptr);
    }
}

/**
 * Enqueues an event. It will be executed at end of update.
 */
static void enqueueEvent(Actor *ptr, const DelayedEvent &e)
{
    std::pair< DelayedEvents::iterator, bool > p =
        delayedEvents.insert(std::make_pair(ptr, e));
    // Delete events take precedence over other events.
    if (!p.second && e.type == EVENT_REMOVE)
    {
        p.first->second.type = EVENT_REMOVE;
    }
}

void GameState::enqueueInsert(Actor *ptr)
{
    DelayedEvent e = { EVENT_INSERT, 0, 0, 0 };
    enqueueEvent(ptr, e);
}

void GameState::enqueueRemove(Actor *ptr)
{
    DelayedEvent e = { EVENT_REMOVE, 0, 0, 0 };
    enqueueEvent(ptr, e);
}

void GameState::enqueueWarp(Character *ptr, MapComposite *m, int x, int y)
{
    // When the player has just disconnected, better not wait for the pointer
    // to become invalid.
    if (!ptr->isConnected())
    {
        warp(ptr, m, x, y);
        return;
    }

    DelayedEvent e = { EVENT_WARP, x, y, m };
    enqueueEvent(ptr, e);
}

void GameState::sayAround(Actor *obj, const std::string &text)
{
    Point speakerPosition = obj->getPosition();
    int visualRange = Configuration::getValue("game_visualRange", 448);

    for (CharacterIterator i(obj->getMap()->getAroundActorIterator(obj, visualRange)); i; ++i)
    {
        if (speakerPosition.inRangeOf((*i)->getPosition(), visualRange))
        {
            sayTo(*i, obj, text);
        }
    }
}

void GameState::sayTo(Actor *destination, Actor *source, const std::string &text)
{
    if (destination->getType() != OBJECT_CHARACTER)
        return; //only characters will read it anyway

    MessageOut msg(GPMSG_SAY);
    if (source == NULL)
    {
        msg.writeInt16(0);
    }
    else if (!source->canMove())
    {
        msg.writeInt16(65535);
    }
    else
    {
        msg.writeInt16(static_cast< Actor * >(source)->getPublicID());
    }
    msg.writeString(text);

    gameHandler->sendTo(static_cast< Character * >(destination), msg);
}

void GameState::sayToAll(const std::string &text)
{
    MessageOut msg(GPMSG_SAY);

    // The message will be shown as if it was from the server
    msg.writeInt16(0);
    msg.writeString(text);

    // Sends it to everyone connected to the game server
    gameHandler->sendToEveryone(msg);
}


std::string GameState::getVariable(const std::string &key)
{
    std::map<std::string, std::string>::iterator iValue =
                                                     mScriptVariables.find(key);
    if (iValue != mScriptVariables.end())
        return iValue->second;
    else
        return std::string();
}

void GameState::setVariable(const std::string &key, const std::string &value)
{
    if (mScriptVariables[key] == value)
        return;
    mScriptVariables[key] = value;
    accountHandler->updateWorldVar(key, value);
    callVariableCallbacks(key, value);
}

void GameState::setVariableFromDbserver(const std::string &key,
                                        const std::string &value)
{
    if (mScriptVariables[key] == value)
        return;
    mScriptVariables[key] = value;
    callVariableCallbacks(key, value);
}

void GameState::callVariableCallbacks(const std::string &key,
                                      const std::string &value)
{
    const MapManager::Maps &maps = MapManager::getMaps();
    for (MapManager::Maps::const_iterator m = maps.begin(),
         m_end = maps.end(); m != m_end; ++m)
    {
        m->second->callWorldVariableCallback(key, value);
    }
}
