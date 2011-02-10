-------------------------------------------------------------
-- Mana Support Library Constants                          --
--                                                         --
-- Some useful numeric values for use by other scripts.    --
--                                                         --
----------------------------------------------------------------------------------
--  Copyright 2008 The Mana World Development Team                              --
--                                                                              --
--  This file is part of The Mana Server.                                       --
--                                                                              --
--  The Mana Server is free software; you can redistribute  it and/or modify it --
--  under the terms of the GNU General  Public License as published by the Free --
--  Software Foundation; either version 2 of the License, or any later version. --
----------------------------------------------------------------------------------

TILESIZE = 32;
HOURS = 3600;
MINUTES = 60;

TYPE_ITEM = 0;
TYPE_ACTOR = 1;
TYPE_NPC = 2;
TYPE_MONSTER = 3;
TYPE_CHARACTER = 4;
TYPE_EFFECT = 5;
TYPE_OTHER = 6;

ACTION_STAND = 0;
ACTION_WALK = 1;
ACTION_ATTACK = 2;
ACTION_SIT = 3;
ACTION_DEAD = 4;
ACTION_HURT = 5;

DIRECTION_DOWN = 1;
DIRECTION_LEFT = 2;
DIRECTION_UP = 4;
DIRECTION_RIGHT = 8;

GENDER_MALE = 0;
GENDER_FEMALE = 1;

DAMAGE_PHYSICAL = 0;
DAMAGE_MAGICAL = 1;
DAMAGE_OTHER = 2;

ELEMENT_NEUTRAL = 0;
ELEMENT_FIRE = 1;
ELEMENT_WATER = 2;
ELEMENT_EARTH = 3;
ELEMENT_AIR = 4;
ELEMENT_LIGHTNING = 5;
ELEMENT_METAL = 6;
ELEMENT_WOOD = 7;
ELEMENT_ICE = 8;

-- Core attributes Id
ATTR_STR = 1;
ATTR_AGI = 2;
ATTR_VIT = 3;
ATTR_INT = 4;
ATTR_DEX = 5;
ATTR_WIL = 6;

-- Derived attributes Id
ATTR_ACCURACY = 6;
ATTR_DEFENSE = 8;
ATTR_DODGE = 9;

ATTR_MAGIC_DODGE = 10;
ATTR_MAGIC_DEFENSE = 11;
ATTR_BONUS_ASPD = 12;

ATTR_HP = 13;
ATTR_MAX_HP = 14;
ATTR_HP_REGEN = 15;

ATTR_MOVE_SPEED_TPS = 16;
ATTR_MOVE_SPEED_RAW = 17;

ATTR_GP = 18;
ATTR_INV_CAPACITY = 19;

MOB_ATTR_PHY_ATK_MIN = 20;
MOB_ATTR_PHY_ATK_DELTA = 21;
MOB_ATTR_MAG_ATK = 22;

-- Emotes - TODO: should be obtainable in a smarter way
EMOTE_DISGUST = 10000;
EMOTE_SURPRISE = 10001;
EMOTE_HAPPY = 10002;
EMOTE_SAD = 10003;
EMOTE_EVIL =  10004;
EMOTE_WINK = 10005;
EMOTE_ANGEL = 10006;
EMOTE_BLUSH = 10007;
EMOTE_TONGUE = 10008;
EMOTE_GRIN = 10009;
EMOTE_UPSET = 10010;
EMOTE_PERTURBED = 10011;
EMOTE_SPEECH = 10012;
EMOTE_BLAH = 10013;

-- Skills - TODO: should be obtainable in a smarter way
SKILL_WEAPON_NONE = 100
SKILL_WEAPON_KNIFE = 101
SKILL_WEAPON_SWORD = 102
SKILL_WEAPON_POLEARM = 103
SKILL_WEAPON_STAFF = 104
SKILL_WEAPON_WHIP = 105
SKILL_WEAPON_BOW = 106
SKILL_WEAPON_SHOOTING = 107
SKILL_WEAPON_MACE = 108
SKILL_WEAPON_AXE = 109
SKILL_WEAPON_THROWN = 110
