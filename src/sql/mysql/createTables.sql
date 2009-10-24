--
-- table: `mana_accounts`
--

CREATE TABLE IF NOT EXISTS `mana_accounts` (
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
-- table: `mana_characters`
--

CREATE TABLE IF NOT EXISTS `mana_characters` (
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
    --
    PRIMARY KEY (`id`),
    UNIQUE KEY `name` (`name`),
    KEY `user_id` (`user_id`),
    FOREIGN KEY (`user_id`)
    	REFERENCES `mana_accounts` (`id`)
    	ON DELETE CASCADE
) ENGINE=InnoDB
DEFAULT CHARSET=utf8
AUTO_INCREMENT=1 ;

--
-- table: `mana_char_skills`
--
CREATE TABLE IF NOT EXISTS `mana_char_skills` (
    `char_id`      int(10)      unsigned NOT NULL,
    `skill_id`     smallint(5)  unsigned NOT NULL,
    `skill_exp`    smallint(5)  unsigned NOT NULL,
    --
    PRIMARY KEY (`char_id`, `skill_id`),
    FOREIGN KEY (`char_id`)
        REFERENCES `mana_characters` (`id`)
        ON DELETE CASCADE
) ENGINE=InnoDB
DEFAULT CHARSET=utf8;

--
-- table: `mana_char_status_effects`
--
CREATE TABLE IF NOT EXISTS `mana_char_status_effects` (
    `char_id`       int(10)         unsigned NOT NULL,
    `status_id`     smallint(5)     unsigned NOT NULL,
    `status_time`   int(10)         signed NOT NULL,
    --
    PRIMARY KEY (`char_id`, `status_id`),
    FOREIGN KEY (`char_id`)
        REFERENCES `mana_characters` (`id`)
        ON DELETE CASCADE
) ENGING=InnoDB
DEFAULT CHARSET=utf8;




--
-- table: `mana_items`
--
CREATE TABLE IF NOT EXISTS `mana_items` (
    `id`           int(10)      unsigned NOT NULL auto_increment,
    `name`         varchar(100)          NOT NULL,
    `description`  varchar(255)          NOT NULL,
    `image`        varchar(50)           NOT NULL,
    `weight`       smallint(5)  unsigned NOT NULL,
    `itemtype`     varchar(50)           NOT NULL,
    `effect`       varchar(100)              NULL,
    `dyestring`    varchar(50)               NULL,
    --
    PRIMARY KEY (`id`),
    KEY `itemtype` (`itemtype`)
) ENGINE=InnoDB
DEFAULT CHARSET=utf8
AUTO_INCREMENT=1 ;

--
-- table: `mana_item_instances`
--
CREATE TABLE IF NOT EXISTS `mana_item_instances` (
    `item_id`      int(10)      unsigned NOT NULL auto_increment,
    `itemclass_id` int(10)      unsigned NOT NULL,
    `amount`       tinyint(3)   unsigned NOT NULL,
    --
    PRIMARY KEY (`item_id`),
    FOREIGN KEY (`itemclass_id`)
        REFERENCES `mana_items` (`id`)
) ENGINE=InnoDB
DEFAULT CHARSET=utf8
AUTO_INCREMENT=1 ;

--
-- table: `mana_item_attributes`
--
CREATE TABLE IF NOT EXISTS `mana_item_attributes` (
    `attribute_id`    int(10)      unsigned NOT NULL auto_increment,
    `item_id`         int(10)      unsigned NOT NULL,
    `attribute_class` tinyint(3)   unsigned NOT NULL,
    `attribute_value` varchar(500)              NULL,
    --
    PRIMARY KEY (`attribute_id`),
    FOREIGN KEY (`item_id`)
        REFERENCES `mana_item_instances` (`item_id`)
        ON DELETE CASCADE
) ENGINE=InnoDB
DEFAULT CHARSET=utf8
AUTO_INCREMENT=1 ;

--
-- table: `mana_inventories`
-- todo: remove class_id and amount and reference on mana_item_instances
--
CREATE TABLE IF NOT EXISTS `mana_inventories` (
    `id`           int(10)      unsigned NOT NULL auto_increment,
    `owner_id`     int(10)      unsigned NOT NULL,
    `slot`         tinyint(3)   unsigned NOT NULL,
    `class_id`     int(10)      unsigned NOT NULL,
    `amount`       tinyint(3)   unsigned NOT NULL,
    --
    PRIMARY KEY (`id`),
    UNIQUE KEY `owner_id` (`owner_id`, `slot`),
    FOREIGN KEY (`owner_id`)
    	REFERENCES `mana_characters` (`id`)
    	ON DELETE CASCADE
) ENGINE=InnoDB
DEFAULT CHARSET=utf8
AUTO_INCREMENT=1 ;

--
-- table: `mana_world_states`
--
CREATE TABLE IF NOT EXISTS `mana_world_states` (
    `state_name`   varchar(100)          NOT NULL,
    `map_id`       int(10)      unsigned default NULL,
    `value`        varchar(255)          default NULL,
    `moddate`      int(10)      unsigned NOT NULL,
    --
    KEY `state_name` (`state_name`)
) ENGINE=InnoDB
DEFAULT CHARSET=utf8;

INSERT INTO mana_world_states VALUES('accountserver_startup',NULL,NULL,UNIX_TIMESTAMP());
INSERT INTO mana_world_states VALUES('accountserver_version',NULL,NULL,UNIX_TIMESTAMP());
INSERT INTO mana_world_states VALUES('database_version',     NULL,'3', UNIX_TIMESTAMP());

--
-- table: `mana_guilds`
--
CREATE TABLE IF NOT EXISTS `mana_guilds` (
    `id`           int(10)      unsigned NOT NULL auto_increment,
    `name`         varchar(35)           NOT NULL,
    --
    PRIMARY KEY (`id`),
    UNIQUE KEY `name` (`name`)
) ENGINE=InnoDB
DEFAULT CHARSET=utf8
AUTO_INCREMENT=1 ;

--
-- table: `mana_guild_members`
--
CREATE TABLE IF NOT EXISTS `mana_guild_members` (
	`guild_id`     int(10)      unsigned NOT NULL,
	`member_id`    int(10)      unsigned NOT NULL,
	`rights`       int(10)      unsigned NOT NULL,
	--
	PRIMARY KEY (`guild_id`, `member_id`),
	FOREIGN KEY (`guild_id`)
		REFERENCES `mana_guilds` (`id`)
		ON DELETE CASCADE,
	FOREIGN KEY (`member_id`)
		REFERENCES `mana_characters` (`id`)
		ON DELETE CASCADE
) ENGINE=InnoDB
DEFAULT CHARSET=utf8;

--
-- table: `mana_quests`
--

CREATE TABLE IF NOT EXISTS `mana_quests` (
	`owner_id`     int(10)      unsigned NOT NULL,
	`name`         varchar(100)          NOT NULL,
	`value`        varchar(200)          NOT NULL,
	--
	PRIMARY KEY (`owner_id`, `name`),
	FOREIGN KEY (`owner_id`)
		REFERENCES `mana_characters` (`id`)
		ON DELETE CASCADE
) ENGINE=InnoDB
DEFAULT CHARSET=utf8;

--
-- table: `mana_auctions`
--

CREATE TABLE IF NOT EXISTS `mana_auctions` (
	`auction_id`    int(10)      unsigned  NOT NULL auto_increment,
	`auction_state` tinyint(3)   unsigned  NOT NULL,
	`char_id`       int(10)      unsigned  NOT NULL,
	`itemclass_id`  int(10)      unsigned  NOT NULL,
	`amount`        int(10)      unsigned  NOT NULL,
	`start_time`    int(10)      unsigned  NOT NULL,
	`end_time`      int(10)      unsigned  NOT NULL,
	`start_price`   int(10)      unsigned  NOT NULL,
	`min_price`     int(10)      unsigned      NULL,
	`buyout_price`  int(10)      unsigned      NULL,
	`description`   varchar(255)               NULL,
	--
	PRIMARY KEY (`auction_id`),
	KEY (`auction_state`),
	KEY (`itemclass_id`),
	KEY (`char_id`),
	FOREIGN KEY (`char_id`)
		REFERENCES `mana_characters` (`id`)
		ON DELETE CASCADE
) ENGINE=InnoDB
DEFAULT CHARSET=utf8
AUTO_INCREMENT=1 ;

--
-- table: `mana_auction_bids`
--

CREATE TABLE IF NOT EXISTS `mana_auction_bids` (
	`bid_id`        int(10)      unsigned  NOT NULL auto_increment,
	`auction_id`    int(10)      unsigned  NOT NULL,
	`char_id`       int(10)      unsigned  NOT NULL,
	`bid_time`      int(10)      unsigned  NOT NULL,
	`bid_price`     int(10)      unsigned  NOT NULL,
	--
	PRIMARY KEY (`bid_id`),
	KEY (`auction_id`),
	KEY (`char_id`),
	FOREIGN KEY (`char_id`)
		REFERENCES `mana_characters` (`id`)
		ON DELETE CASCADE
) ENGINE=InnoDB
DEFAULT CHARSET=utf8
AUTO_INCREMENT=1 ;

--
-- table: `mana_post`
--

CREATE TABLE IF NOT EXISTS `mana_post` (
	`letter_id`        int(10)      unsigned  NOT NULL auto_increment,
	`sender_id`        int(10)      unsigned  NOT NULL,
	`receiver_id`      int(10)      unsigned  NOT NULL,
	`letter_type`      int(5)       unsigned  NOT NULL,
	`expiration_date`  int(10)      unsigned  NOT NULL,
	`sending_date`     int(10)      unsigned  NOT NULL,
	`letter_text`      TEXT                       NULL,
	--
	PRIMARY KEY (`letter_id`),
	INDEX `fk_letter_sender` (`sender_id` ASC) ,
	INDEX `fk_letter_receiver` (`receiver_id` ASC) ,
	--
	FOREIGN KEY (`sender_id` )
		REFERENCES `mana_characters` (`id`)
		ON DELETE CASCADE,
    FOREIGN KEY (`receiver_id` )
		REFERENCES `mana_characters` (`id`)
		ON DELETE CASCADE
) ENGINE = InnoDB
DEFAULT CHARSET=utf8
AUTO_INCREMENT=1 ;

--
-- table: `mana_post_attachements`
--

CREATE TABLE IF NOT EXISTS `mana_post_attachments` (
	`attachment_id`    int(10)      unsigned  NOT NULL auto_increment,
	`letter_id`        int(10)      unsigned  NOT NULL,
	`item_id`          int(10)      unsigned  NOT NULL,
	--
	PRIMARY KEY (`attachment_id`) ,
	INDEX `fk_attachment_letter` (`letter_id` ASC) ,
	INDEX `fk_attachment_item` (`item_id` ASC),
	--
	FOREIGN KEY (`letter_id` )
		REFERENCES `mana_post` (`letter_id`)
		ON DELETE CASCADE,
	FOREIGN KEY (`item_id` )
		REFERENCES `mana_item_instances` (`item_id`)
		ON DELETE RESTRICT
) ENGINE = InnoDB
DEFAULT CHARSET=utf8
AUTO_INCREMENT=1 ;

--
-- table: `mana_online_list`
--

CREATE TABLE IF NOT EXISTS `mana_online_list` (
    `char_id`      int(10)      unsigned NOT NULL,
    `login_date`   int(10)      NOT NULL,
    --
    PRIMARY KEY (`char_id`),
    FOREIGN KEY (`char_id`)
    	REFERENCES `mana_characters` (`id`)
    	ON DELETE CASCADE
) ENGINE=InnoDB
DEFAULT CHARSET=utf8 ;

-- create a view to show more details about online users
CREATE VIEW mana_v_online_chars
AS
   SELECT l.char_id    as char_id,
          l.login_date as login_date,
          c.user_id    as user_id,
          c.name       as name,
          c.gender     as gender,
          c.level      as level,
          c.map_id     as map_id
     FROM mana_online_list l
     JOIN mana_characters c
       ON l.char_id = c.id;


CREATE TABLE IF NOT EXISTS `mana_transactions` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `char_id` int(11) NOT NULL,
  `action` int(11) NOT NULL,
  `message` text,
  `time` int(11) NOT NULL,
  PRIMARY KEY  (`id`)
)
AUTO_INCREMENT=0 ;
