/*
 *  The Mana World
 *  Copyright 2008 The Mana World Development Team
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
 *  $Id: $
 */

#include "encryption.h"

#include <time.h>

std::string Encryption::GetSHA2Hash(std::string stringToHash)
{

    if (stringToHash.length() == 0)
        return "";

    sha2 mySha;
    return mySha.GetHash(shaType, (const sha_byte *)stringToHash.c_str(), stringToHash.size());
}

char _getRandomCharacter()
{
    char result = '0';

    // Taking a number of character taken between 33 and 127
    // (Every normal characters from ASCII table).
    int number = (rand() % 94) + 33;

    // Those characters are dodged to ease user input and avoid database
    // breaks: " ' , ` \ ^ * / ~ |
    if (number == 34 || number == 39 || number == 42 || number == 44 ||
        number == 47 || number == 92 || number == 94 || number == 96 ||
        number == 124 || number == 126)
        number++;

    result = char(number);
    return result;
}

/**
  * Using this function, the random salt changes at every second.
  */
std::string Encryption::CreateRandomPassword()
{

    std::string result = "";

    // Ititializing random seed.
    srand(time(NULL));
    // Taking a number of character taken between 20 and 30.
    int characterNumber = (rand() % 10) + 20;

    for (int a = 1; a < characterNumber; a++)
    {
        result += _getRandomCharacter();
    }
        return result;
}
