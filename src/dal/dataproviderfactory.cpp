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
 *
 *  $Id$
 */


#include "dataproviderfactory.h"

#if defined (MYSQL_SUPPORT)
#include "mysqldataprovider.h"
#elif defined (POSTGRESQL_SUPPORT)
#include "pqdataprovider.h"
#elif defined (SQLITE_SUPPORT)
#include "sqlitedataprovider.h"
#else
#error "no database backend defined"
#endif


namespace tmwserv
{
namespace dal
{


/**
 * Default constructor.
 */
DataProviderFactory::DataProviderFactory(void)
    throw()
{
    // NOOP
}


/**
 * Destructor.
 */
DataProviderFactory::~DataProviderFactory(void)
    throw()
{
    // NOOP
}


/**
 * Create a data provider.
 */
DataProvider*
DataProviderFactory::createDataProvider(void)
{
#if defined (MYSQL_SUPPORT)
    MySqlDataProvider* provider = new MySqlDataProvider;
    return provider;
#elif defined (POSTGRESQL_SUPPORT)
    PqDataProvider *provider = new PqDataProvider;
    return provider;
#else // SQLITE_SUPPORT
    SqLiteDataProvider* provider = new SqLiteDataProvider;
    return provider;
#endif
}


} // namespace dal
} // namespace tmwserv
