--
-- table: `tmw_accounts`
--

CREATE TABLE IF NOT EXISTS `tmw_accounts` (
    `id`           int(10)      unsigned NOT NULL auto_increment,
    `username`     varchar(64)           NOT NULL,
    `password`     varchar(64)           NOT NULL,
    `email`        varchar(32)           NOT NULL,
    `level`        tinyint(3)   unsigned NOT NULL,
    `banned`       int(10)      unsigned NOT NULL,
    `registration` int(10)      unsigned NOT NULL,
    `lastlogin`    int(10)      unsigned NOT NULL,
    --
    PRIMARY KEY  (`id`),
    UNIQUE KEY `username` (`username`),
    UNIQUE KEY `email` (`email`)
) ENGINE=InnoDB 
DEFAULT CHARSET=utf8
AUTO_INCREMENT=1 ;

--
-- table: `tmw_characters`
--

CREATE TABLE IF NOT EXISTS `tmw_characters` (
    `id`           int(10)      unsigned NOT NULL auto_increment,
    `user_id`      int(10)      unsigned NOT NULL,
	`name`         varchar(32)           NOT NULL,
    --
    `gender`       tinyint(3)   unsigned NOT NULL,
    `hair_style`   tinyint(3)   unsigned NOT NULL,
    `hair_color`   tinyint(3)   unsigned NOT NULL,
    `level`        tinyint(3)   unsigned NOT NULL,
    `char_pts`     smallint(5)  unsigned NOT NULL,
    `correct_pts`  smallint(5)  unsigned NOT NULL,
    `money`        int(10)      unsigned NOT NULL,
    -- location on the map
    `x`            smallint(5)  unsigned NOT NULL,
    `y`            smallint(5)  unsigned NOT NULL,
    `map_id`       tinyint(3)   unsigned NOT NULL,
    -- attributes
    `str`          smallint(5)  unsigned NOT NULL,
    `agi`          smallint(5)  unsigned NOT NULL,
    `dex`          smallint(5)  unsigned NOT NULL,
    `vit`          smallint(5)  unsigned NOT NULL,
    `int`          smallint(5)  unsigned NOT NULL,
    `will`         smallint(5)  unsigned NOT NULL,
    -- skill experience
    `unarmed_exp`  smallint(5)  unsigned NOT NULL,
    `knife_exp`    smallint(5)  unsigned NOT NULL,
    `sword_exp`    smallint(5)  unsigned NOT NULL,
    `polearm_exp`  smallint(5)  unsigned NOT NULL,
    `staff_exp`    smallint(5)  unsigned NOT NULL,
    `whip_exp`     smallint(5)  unsigned NOT NULL,
    `bow_exp`      smallint(5)  unsigned NOT NULL,
    `shoot_exp`    smallint(5)  unsigned NOT NULL,
    `mace_exp`     smallint(5)  unsigned NOT NULL,
    `axe_exp`      smallint(5)  unsigned NOT NULL,
    `thrown_exp`   smallint(5)  unsigned NOT NULL,
    --
    PRIMARY KEY (`id`),
    UNIQUE KEY `name` (`name`),
    KEY `user_id` (`user_id`),
    FOREIGN KEY (`user_id`) 
    	REFERENCES `tmw_accounts` (`id`)
    	ON DELETE CASCADE 
) ENGINE=InnoDB
DEFAULT CHARSET=utf8 
AUTO_INCREMENT=1 ;
    
--
-- table: `tmw_inventories`
--

CREATE TABLE IF NOT EXISTS `tmw_inventories` (
    `id`           int(10)      unsigned NOT NULL auto_increment,
    `owner_id`     int(10)      unsigned NOT NULL,
    `slot`         tinyint(3)   unsigned NOT NULL,
    `class_id`     int(10)      unsigned NOT NULL,
    `amount`       tinyint(3)   unsigned NOT NULL,
    --
    PRIMARY KEY (`id`),
    UNIQUE KEY `owner_id` (`owner_id`, `slot`),
    FOREIGN KEY (`owner_id`)
    	REFERENCES `tmw_characters` (`id`)
    	ON DELETE CASCADE 
) ENGINE=InnoDB
DEFAULT CHARSET=utf8 
AUTO_INCREMENT=1 ;

--
-- table: `tmw_world_states`
--

CREATE TABLE IF NOT EXISTS `tmw_world_states` (
    `state_name`   varchar(100)          NOT NULL,
    `map_id`       int(10)      unsigned default NULL,
    `value`        varchar(255)          NOT NULL,
    `moddate`      int(10)      unsigned NOT NULL,
    --
    KEY `state_name` (`state_name`)
) ENGINE=InnoDB
DEFAULT CHARSET=utf8;

--
-- table: `tmw_guilds`
--

CREATE TABLE IF NOT EXISTS `tmw_guilds` (
    `id`           int(10)      unsigned NOT NULL auto_increment,
    `name`         varchar(35)           NOT NULL,
    --
    PRIMARY KEY (`id`),
    UNIQUE KEY `name` (`name`)
) ENGINE=InnoDB
DEFAULT CHARSET=utf8
AUTO_INCREMENT=1 ;

--
-- table: `tmw_guild_members`
--

CREATE TABLE IF NOT EXISTS `tmw_guild_members` (
	`guild_id`     int(10)      unsigned NOT NULL,
	`member_id`    int(10)      unsigned NOT NULL,
	`rights`       int(10)      unsigned NOT NULL,
	--
	PRIMARY KEY (`guild_id`, `member_id`),
	FOREIGN KEY (`guild_id`)
		REFERENCES `tmw_guilds` (`id`)
		ON DELETE CASCADE,
	FOREIGN KEY (`member_id`)
		REFERENCES `tmw_characters` (`id`)
		ON DELETE CASCADE
) ENGINE=InnoDB
DEFAULT CHARSET=utf8;
        
--
-- table: `tmw_quests`
--

CREATE TABLE IF NOT EXISTS `tmw_quests` (
	`owner_id`     int(10)      unsigned NOT NULL,
	`name`         varchar(100)          NOT NULL,
	`value`        varchar(200)          NOT NULL,
	--
	PRIMARY KEY (`owner_id`, `name`),
	FOREIGN KEY (`owner_id`)
		REFERENCES `tmw_characters` (`id`)
		ON DELETE CASCADE
) ENGINE=InnoDB
DEFAULT CHARSET=utf8;
