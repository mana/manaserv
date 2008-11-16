/*
 *  The Mana World Server
 *  Copyright 2007 The Mana World Development Team
 *
 *  This file is part of The Mana World.
 *
 *  The Mana World  is free software; you can redistribute  it and/or modify it
 *  under the terms of the GNU General  Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or any later version.
 *
 *  The Mana  World is  distributed in  the hope  that it  will be  useful, but
 *  WITHOUT ANY WARRANTY; without even  the implied warranty of MERCHANTABILITY
 *  or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 *  more details.
 *
 *  You should  have received a  copy of the  GNU General Public  License along
 *  with The Mana  World; if not, write to the  Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "utils/processorutils.hpp"

bool utils::processor::isLittleEndian;

void utils::processor::init()
{
    utils::processor::isLittleEndian = utils::processor::littleEndianCheck();
}

bool utils::processor::littleEndianCheck()
{
   short int word = 0x0001;     // Store 0x0001 in a 16-bit int.
   char *byte = (char *) &word; // 'byte' points to the first byte in word.
   return(byte[0]);          // byte[0] will be 1 on little-endian processors.
}
