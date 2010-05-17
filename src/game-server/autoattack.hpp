#ifndef AUTOATTACK_HPP
#define AUTOATTACK_HPP

#include <list>
#include <limits>

/**
 * Methods of damage calculation
 */
enum DMG_TY
{
    DAMAGE_PHYSICAL = 0,
    DAMAGE_MAGICAL,
    DAMAGE_DIRECT,
    DAMAGE_OTHER = -1
};

/**
 * Structure that describes the severity and nature of an attack a being can
 * be hit by.
 */
struct Damage
{
    unsigned short base;            /**< Base amount of damage. */
    unsigned short delta;           /**< Additional damage when lucky. */
    unsigned short cth;             /**< Chance to hit. Opposes the evade attribute. */
    unsigned char element;          /**< Elemental damage. */
    DMG_TY type;                    /**< Damage type: Physical or magical? */
    unsigned trueStrike : 1;        /**< Override dodge calculation */
    std::list<size_t> usedSkills;   /**< Skills used by source (needed for exp calculation) */
    unsigned short range;           /**< Maximum distance that this attack can be used from */
    Damage(unsigned short base, unsigned short delta, unsigned short cth,
           unsigned char element, DMG_TY type = DAMAGE_OTHER,
           unsigned short range = std::numeric_limits<unsigned short>::max())
    : base(base), delta(delta), cth(cth), element(element), type(type),
    trueStrike(false), range(range) {}
};

/**
 * Class that stores information about an auto-attack
 */

class AutoAttack
{
    public:
        AutoAttack(Damage &damage, unsigned int delay, unsigned int warmup)
                : mDamage(damage), mTimer(0), mAspd(delay),
                mWarmup(warmup && warmup < delay ? warmup : delay >> 2) {}
        unsigned short getTimer() const { return mTimer; }
        bool tick() { return mTimer ? !--mTimer : false; }
        void reset() { mTimer = mAspd; }
        bool operator<(const AutoAttack &rhs) const
        { return mTimer < rhs.getTimer(); }
        bool isReady() const { return !(mTimer - mWarmup); }
        void halt() { if (mTimer >= mWarmup) mTimer = 0; }
        void softReset() { if (mTimer >= mWarmup) mTimer = mAspd; }
        const Damage &getDamage() const { return mDamage; }
    private:
        Damage mDamage;
        /**
         * Internal timer that is modified each tick.
         * When > warmup, the attack is warming up before a strike
         * When = warmup, the attack triggers, dealing damage to the target *if* the target is still in range.
         *  (The attack is canceled when the target moves out of range before the attack can hit, there should be a trigger for scripts here too)
         *  (Should the character automatically persue when the target is still visible in this case?)
         * When < warmup, the attack is cooling down after a strike. When in cooldown, the timer should not be soft-reset.
         * When 0, the attack is inactive (the character is doing something other than attacking and the attack is not in cooldown)
         */
        unsigned short mTimer;
        unsigned short mAspd; // Value to reset the timer to (warmup + cooldown)
        /**
         * Pre-attack delay tick.
         * This MUST be smaller than or equal to the aspd!
         * So the attack triggers where timer == warmup, having gone through aspd - warmup ticks.
         */
        unsigned short mWarmup;
};

/**
 * Helper class for storing multiple auto-attacks.
 */
class AutoAttacks
{
    public:

        /**
         * Whether the being has at least one auto attack that is ready.
         */
        void add(AutoAttack n);
        void clear(); // Wipe the list completely (used in place of remove for now; FIXME)
        void start();
        void stop(); // If the character does some action other than attacking, reset all warmups (NOT cooldowns!)
        void tick(std::list<AutoAttack> *ret = 0);
    private:
        bool mActive; /**< Marks whether or not to keep auto-attacking. Cooldowns still need to be processed when false. */
        std::list < AutoAttack > mAutoAttacks;

};

#endif // AUTOATTACK_HPP
