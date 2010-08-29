#include "autoattack.hpp"

void AutoAttacks::add(AutoAttack n)
{
    mAutoAttacks.push_back(n);
    // Slow, but safe.
    mAutoAttacks.sort();
}

void AutoAttacks::clear()
{
    mAutoAttacks.clear();
}

void AutoAttacks::stop()
{
    for (std::list<AutoAttack>::iterator it = mAutoAttacks.begin(); it != mAutoAttacks.end(); ++it)
        it->halt();
    mActive = false;
}

void AutoAttacks::start()
{
    for (std::list<AutoAttack>::iterator it = mAutoAttacks.begin(); it != mAutoAttacks.end(); ++it)
        it->softReset();
    mActive = true;
}

void AutoAttacks::tick(std::list<AutoAttack> *ret)
{
    for (std::list<AutoAttack>::iterator it = mAutoAttacks.begin(); it != mAutoAttacks.end(); ++it)
        if (it->tick()) {
            if (mActive)
                it->reset();
            else
                it->halt();
        } else if (ret && it->isReady())
            ret->push_back(*it);
}
