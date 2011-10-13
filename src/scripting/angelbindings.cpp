/*
 *  The Mana Server
 *  Copyright (C) 2007-2011  The Mana World Development Team
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

/*
 * This file includes all script bindings available to angel scripts.
 * When you add or change a script binding please document it on
 *
 * http://doc.manasource.org/scripting <-- TODO: right place on wiki to do that
 */

#include "scripting/angelscript/scriptstdstring/scriptstdstring.h"

/**
 * Raises an angel script error if called from an angel script binding
 */
void raiseScriptError(const char* description)
{
    LOG_WARN("Angel script error: "<< description);
    asIScriptContext *ctx = asGetActiveContext();
    ctx->SetException(description);
}

/**
 * log(int logLevel, string logMessage): void
 * Logs the given message to the log.
 */
void log(const int logLevel, const std::string str)
{
    if (logLevel >= utils::Logger::Fatal && logLevel <= utils::Logger::Debug)
         utils::Logger::output(str, (utils::Logger::Level) logLevel);
    else
        raiseScriptError("log called with unknown loglevel");
}
