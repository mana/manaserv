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

#ifndef SKILL_H
#define SKILL_H

#include <iostream>
#include <vector>

#ifdef SCRIPT_SUPPORT
#include "script.h"
#endif

class Skill
{
    /*
     * Skill identifier
     */
    std::string id;

    /*
     * Skill description
     */
    std::string description;

    /*
     * Children skills
     */
    std::vector<Skill*> children;

    /*
     * Skill properties/weighting (used when calculating player class)
     */
    float light;
    float dark;
    float life;
    float death;

    /*
     * Skill script
     */
#ifdef SCRIPT_SUPPORT
    Script *script;
#endif

  public:
    Skill(const std::string &ident);
    virtual ~Skill();

    /*
     * addSkill
     * Add skill to parent with id
     */
    bool addSkill(const std::string &, Skill *);

    /*
     * deleteSkill
     * Delete skill from tree with id
     */
    bool deleteSkill(const std::string &, bool delTree = false);

    /*
     * useSkill
     * Uses skill (runs skill script). Returns true upon successful
     * skill completion, false otherwise.
     */
    bool useSkill();

    /*
     * setScript
     * Set script for the skill to execute when used.
     */
    bool setScript(const std::string &);

    /*
     * printTree
     * Print tree to stdout
     */
    void printTree(const std::string &indent) {
	std::cerr << indent << id << std::endl;
	for (unsigned int i = 0; i < children.size(); i++) {
	    children[i]->printTree(indent + "  ");
	}
    }
};

#endif

