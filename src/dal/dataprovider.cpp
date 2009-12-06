/*
 *  The Mana World Server
 *  Copyright 2004 The Mana World Development Team
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

#include "dataprovider.h"
#include "../utils/logger.h"

namespace dal
{


/**
 * Constructor.
 */
DataProvider::DataProvider()
    throw()
        : mIsConnected(false),
          mRecordSet()
{
}


/**
 * Destructor.
 */
DataProvider::~DataProvider()
    throw()
{
}


/**
 * Get the connection status.
 */
bool DataProvider::isConnected() const
    throw()
{
    return mIsConnected;
}

/**
 * Get the DataBase Name
 */
std::string DataProvider::getDbName() const
{
    if (!isConnected())
    {
        return "";
    }

    return mDbName;
}

/*
std::string& DataProvider::escapeSQL(std::string &sql)
{
    size_t pos = 0;

    pos = sql.find("'", pos);
    while (pos != std::string::npos)
    {
        sql.replace(pos, 1, "\'\'");
        pos += 2;
        pos = sql.find("'", pos);
    }

    return sql;
}
*/

} // namespace dal
