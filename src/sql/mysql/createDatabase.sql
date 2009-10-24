/*
 *  The Mana Server
 *  Copyright 2008 The Mana World Development Team
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

CREATE USER 'mana'@'%' IDENTIFIED BY 'testtest';
CREATE USER 'mana'@'localhost' IDENTIFIED BY 'testtest';

GRANT USAGE ON * . * TO 'mana'@'%' IDENTIFIED BY 'testtest'
	WITH MAX_QUERIES_PER_HOUR 0 MAX_CONNECTIONS_PER_HOUR 0 MAX_UPDATES_PER_HOUR 0 MAX_USER_CONNECTIONS 0 ;
GRANT USAGE ON * . * TO 'mana'@'localhost' IDENTIFIED BY 'testtest'
	WITH MAX_QUERIES_PER_HOUR 0 MAX_CONNECTIONS_PER_HOUR 0 MAX_UPDATES_PER_HOUR 0 MAX_USER_CONNECTIONS 0 ;

CREATE DATABASE IF NOT EXISTS `mana` ;

GRANT ALL PRIVILEGES ON `mana` . * TO 'mana'@'%';
GRANT ALL PRIVILEGES ON `mana` . * TO 'mana'@'localhost';
