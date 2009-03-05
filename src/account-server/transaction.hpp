/*
 *  The Mana World Server
 *  Copyright 2009 The Mana World Development Team
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

#ifndef _TMWSERV_TRANSACTION_H_
#define _TMWSERV_TRANSACTION_H_

struct Transaction
{
    unsigned int mAction;
    unsigned int mCharacterId;
    std::string mMessage;
};

enum
{
    TRANS_CHAR_CREATE = 1,
    TRANS_CHAR_SELECTED,
    TRANS_CHAR_DELETED,
    TRANS_MSG_PUBLIC,
    TRANS_MSG_ANNOUNCE,
    TRANS_MSG_PRIVATE,
    TRANS_CHANNEL_JOIN,
    TRANS_CHANNEL_KICK,
    TRANS_CHANNEL_MODE,
    TRANS_CHANNEL_QUIT,
    TRANS_CHANNEL_LIST,
    TRANS_CHANNEL_USERLIST,
    TRANS_CHANNEL_TOPIC,
};

#endif
