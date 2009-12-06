/*
 *  The Mana Server
 *  Copyright (C) 2008  The Mana World Development Team
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

#include "encryption.h"

#include <time.h>
#include <stdlib.h>

using namespace Encryption;

static char getRandomCharacter()
{
    // Taking a number of character taken between 33 and 127
    // (every normal characters from ASCII table).
    int number = (rand() % 94) + 33;

    // Those characters are dodged to ease user input and avoid database
    // breaks: " ' , ` \ ^ * / ~ |
    if (number == 34 || number == 39 || number == 42 || number == 44 ||
        number == 47 || number == 92 || number == 94 || number == 96 ||
        number == 124 || number == 126)
        number++;

    return (char) number;
}

/**
  * Using this function, the random salt changes at every second.
  */
std::string createRandomPassword()
{
    std::string result = "";

    // Ititializing random seed.
    srand(time(NULL));

    // Taking a number of character taken between 20 and 30.
    int characterNumber = (rand() % 10) + 20;

    for (int a = 1; a < characterNumber; a++)
    {
        result += getRandomCharacter();
    }

    return result;
}
