/*
 *  The Mana World
 *  Copyright 2006 The Mana World Development Team
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

#include <zlib.h>

#include "utils/logger.h"
#include "utils/zlib.hpp"

static void logZlibError(int error)
{
    switch (error)
    {
        case Z_MEM_ERROR:
            LOG_ERROR("Out of memory while decompressing data!");
            break;
        case Z_VERSION_ERROR:
            LOG_ERROR("Incompatible zlib version!");
            break;
        case Z_DATA_ERROR:
            LOG_ERROR("Incorrect zlib compressed data!");
            break;
        default:
            LOG_ERROR("Unknown error while decompressing data!");
    }
}

bool inflateMemory(char *in, unsigned inLength,
                   char *&out, unsigned &outLength)
{
    int bufferSize = 256 * 1024;
    int ret;
    z_stream strm;

    out = (char *)malloc(bufferSize);

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.next_in = (Bytef *)in;
    strm.avail_in = inLength;
    strm.next_out = (Bytef *)out;
    strm.avail_out = bufferSize;

    ret = inflateInit2(&strm, 15 + 32);

    if (ret != Z_OK)
    {
        logZlibError(ret);
        return false;
    }

    do
    {
        ret = inflate(&strm, Z_SYNC_FLUSH);

        switch (ret) {
            case Z_NEED_DICT:
            case Z_STREAM_ERROR:
                ret = Z_DATA_ERROR;
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                inflateEnd(&strm);
                logZlibError(ret);
                return false;
        }

        if (ret != Z_STREAM_END)
        {
            out = (char *)realloc(out, bufferSize * 2);

            if (!out)
            {
                inflateEnd(&strm);
                logZlibError(Z_MEM_ERROR);
                return false;
            }

            strm.next_out = (Bytef *)(out + bufferSize);
            strm.avail_out = bufferSize;
            bufferSize *= 2;
        }
    }
    while (ret != Z_STREAM_END);

    if (strm.avail_in != 0)
    {
        logZlibError(Z_DATA_ERROR);
        return false;
    }

    outLength = bufferSize - strm.avail_out;
    inflateEnd(&strm);
    return true;
}
