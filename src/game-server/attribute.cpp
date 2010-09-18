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

#include "attribute.hpp"
#include "game-server/being.hpp"
#include "utils/logger.h"
#include <cassert>

AttributeModifiersEffect::AttributeModifiersEffect(AT_TY sType, AME_TY eType)
        : mMod(eType == AME_MULT ? 1 : 0), mSType(sType), mEType(eType)
{
    assert(eType == AME_MULT || eType == AME_ADD);
    assert(sType == TY_ST    || sType == TY_NST || sType == TY_NSTB);
    LOG_DEBUG("Layer created with eType " << eType << " and sType " << sType << ".");
}

AttributeModifiersEffect::~AttributeModifiersEffect()
{
    // ?
    /*mStates.clear();*/
    LOG_WARN("DELETION of attribute effect!");
}

bool AttributeModifiersEffect::add(unsigned short duration, double value, double prevLayerValue, int level)
{
    LOG_DEBUG("Adding modifier with value " << value <<
              " with a previous layer value of " << prevLayerValue << ". "
              "Current mod at this layer: " << mMod << ".");
    bool ret = false;
    mStates.push_back(new AttributeModifierState(duration, value, level));
    switch (mSType) {
    case TY_ST:
        switch (mEType) {
        case AME_ADD:
            if (value)
            {
                ret = true;
                mMod += value;
                mCacheVal = prevLayerValue + mMod;
            }
            break;
        case AME_MULT:
            if (value != 1)
            {
                ret = true;
                mMod *= value;
                mCacheVal = prevLayerValue * mMod;
            }
            break;
        default:
            LOG_FATAL("Attribute modifiers effect: unhandled type '"
                      << mEType << "' as a stackable!");
            assert(0);
            break;
        }
        break;
    case TY_NST:
        switch (mEType) {
        case AME_ADD:
            if (value > mMod)
            {
                ret = true;
                mMod = value;
                if (mMod > prevLayerValue)
                    mCacheVal = mMod;
            }
            break;
        default:
            LOG_FATAL("Attribute modifiers effect: unhandled type '"
                      << mEType << "' as a non-stackable!");
            assert(0);
        }
        // A multiplicative type would also be nonsensical
        break;
    case TY_NSTB:
        switch (mEType) {
        case AME_ADD:
        case AME_MULT:
            if (value > mMod)
            {
                ret = true;
                mMod = value;
                mCacheVal = mEType == AME_ADD ? prevLayerValue + mMod
                                              : prevLayerValue * mMod;
            }
            break;
        default:
            LOG_FATAL("Attribute modifiers effect: unhandled type '"
                      << mEType << "' as a non-stackable bonus!");
            assert(0);
        }
        break;
    default:
        LOG_FATAL("Attribute modifiers effect: unknown modifier type '"
                  << mSType << "'!");
        assert(0);
    }
    return ret;
}

bool durationCompare(const AttributeModifierState *lhs,
                     const AttributeModifierState *rhs)
{ return lhs->mDuration < rhs->mDuration; }

bool AttributeModifiersEffect::remove(double value, unsigned int id, bool fullCheck) {
    /* We need to find and check this entry exists, and erase the entry
       from the list too. */
    if (!fullCheck)
        mStates.sort(durationCompare); /* Search only through those with a duration of 0. */
    bool ret = false;
    for (std::list< AttributeModifierState * >::iterator it = mStates.begin();
         it != mStates.end() && (fullCheck || !(*it)->mDuration);
         ++it)
    {
        /* Check for a match */
        if ((*it)->mValue != value || (*it)->mId != id)
            continue;
        if (mSType == TY_ST)
            updateMod();
        delete *it;
        mStates.erase(it);
        if (!id)
            return true;
        else ret = true;
    }
    if (ret && mSType != TY_ST)
        updateMod();
    return ret;
}

void AttributeModifiersEffect::updateMod(double value)
{
    if (mSType == TY_ST)
    {
        if (mEType == AME_ADD)
            mMod -= value;
        else if (mEType == AME_MULT)
            mMod /= value;
        else LOG_ERROR("Attribute modifiers effect: unhandled type '"
                       << mEType << "' as a stackable in cache update!");
    }
    else if (mSType == TY_NST || mSType == TY_NSTB)
    {
        if (mMod == value)
        {
            mMod = 0;
            for (std::list< AttributeModifierState * >::const_iterator
                 it2 = mStates.begin(),
                 it2_end = mStates.end();
                it2 != it2_end;
                ++it2)
                if ((*it2)->mValue > mMod)
                    mMod = (*it2)->mValue;
        }
        else LOG_ERROR("Attribute modifiers effect: unhandled type '"
                       << mEType << "' as a non-stackable in cache update!");
    }
    else LOG_ERROR("Attribute modifiers effect: unknown modifier type '"
                   << mSType << "' in cache update!");
}

bool AttributeModifiersEffect::recalculateModifiedValue(double newPrevLayerValue)
 {
    double oldValue = mCacheVal;
    switch (mEType) {
        case AME_ADD:
            switch (mSType) {
            case TY_ST:
            case TY_NSTB:
                mCacheVal = newPrevLayerValue + mMod;
            break;
            case TY_NST:
                mCacheVal = newPrevLayerValue < mMod ? mMod : newPrevLayerValue;
            break;
            default:
            LOG_FATAL("Unknown stack type '" << mEType << "'!");
            assert(0);
        } break;
        case AME_MULT:
            mCacheVal = mSType == TY_ST ? newPrevLayerValue * mMod : newPrevLayerValue * mMod;
        break;
        default:
        LOG_FATAL("Unknown effect type '" << mEType << "'!");
        assert(0);
    }
    return oldValue != mCacheVal;
}

bool Attribute::add(unsigned short duration, double value, unsigned int layer, int level)
{
    assert(mMods.size() > layer);
    LOG_DEBUG("Adding modifier to attribute with duration " << duration <<
              ", value " << value << ", at layer " << layer << " with id "
              << level);
    if (mMods.at(layer)->add(duration, value,
                            (layer ? mMods.at(layer - 1)->getCachedModifiedValue()
                                   : mBase)
                            , level))
    {
        while (++layer < mMods.size())
        {
            if (!mMods.at(layer)->recalculateModifiedValue(
                       mMods.at(layer - 1)->getCachedModifiedValue()))
            {
                LOG_DEBUG("Modifier added, but modified value not changed.");
                return false;
            }
        }
        LOG_DEBUG("Modifier added. Base value: " << mBase << ", new modified "
                  "value: " << getModifiedAttribute() << ".");
        return true;
    }
    LOG_DEBUG("Failed to add modifier!");
    return false;
}

bool Attribute::remove(double value, unsigned int layer, int lvl, bool fullcheck)
{
    assert(mMods.size() > layer);
    if (mMods.at(layer)->remove(value, lvl, fullcheck))
    {
        while (++layer < mMods.size())
           if (!mMods.at(layer)->recalculateModifiedValue(
                         mMods.at(layer - 1)->getCachedModifiedValue()))
               return false;
        return true;
    }
    return false;
}

bool AttributeModifiersEffect::tick()
{
    bool ret = false;
    std::list<AttributeModifierState *>::iterator it = mStates.begin();
    while (it != mStates.end())
    {
        if ((*it)->tick())
        {
            double value = (*it)->mValue;
            LOG_DEBUG("Modifier of value " << value << " expiring!");
            delete *it;
            mStates.erase(it++);
            updateMod(value);
            ret = true;
        }
        ++it;
    }
    return ret;
}

Attribute::Attribute(const std::vector<struct AttributeInfoType> &type)
{
    LOG_DEBUG("Construction of new attribute with '" << type.size() << "' layers.");
    for (unsigned int i = 0; i < type.size(); ++i)
    {
        LOG_DEBUG("Adding layer with stack type " << type[i].sType << " and effect type " << type[i].eType << ".");
        mMods.push_back(new AttributeModifiersEffect(type[i].sType,
                                                 type[i].eType));
        LOG_DEBUG("Layer added.");
    }
}

Attribute::~Attribute()
{
    for (std::vector<AttributeModifiersEffect *>::iterator it = mMods.begin(),
         it_end = mMods.end(); it != it_end; ++it)
    {
        // ?
        //delete *it;
    }
}

bool Attribute::tick()
{
    bool ret = false;
    double prev = mBase;
    for (std::vector<AttributeModifiersEffect *>::iterator it = mMods.begin(),
        it_end = mMods.end(); it != it_end; ++it)
    {
        if ((*it)->tick())
        {
            LOG_DEBUG("Attribute layer " << mMods.begin() - it
                      << " has expiring modifiers.");
            ret = true;
        }
        if (ret)
            if (!(*it)->recalculateModifiedValue(prev)) ret = false;
        prev = (*it)->getCachedModifiedValue();
    }
    return ret;
}

void Attribute::clearMods()
{
    for (std::vector<AttributeModifiersEffect *>::iterator it = mMods.begin(),
        it_end = mMods.end(); it != it_end; ++it)
        (*it)->clearMods(mBase);
}

void Attribute::setBase(double base) {
    LOG_DEBUG("Setting base attribute from " << mBase << " to " << base << ".");
    double prev = mBase = base;
    std::vector<AttributeModifiersEffect *>::iterator it = mMods.begin();
    while (it != mMods.end())
        if ((*it)->recalculateModifiedValue(prev))
            prev = (*it++)->getCachedModifiedValue();
        else
            break;
}

void AttributeModifiersEffect::clearMods(double baseValue)
{
    mStates.clear();
    mCacheVal = baseValue;
    mMod = mEType == AME_ADD ? 0 : 1;
}
