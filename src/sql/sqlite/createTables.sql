CREATE TABLE tmw_accounts 
( 
   id           INTEGER     PRIMARY KEY, 
   username     TEXT        NOT NULL UNIQUE, 
   password     TEXT        NOT NULL, 
   email        TEXT        NOT NULL, 
   level        INTEGER     NOT NULL, 
   banned       INTEGER     NOT NULL, 
   registration INTEGER     NOT NULL, 
   lastlogin    INTEGER     NOT NULL  
);

CREATE INDEX tmw_accounts_username ON tmw_accounts ( username );


CREATE TABLE tmw_characters 
(
   id           INTEGER     PRIMARY KEY,
   user_id      INTEGER     NOT NULL,
   name         TEXT        NOT NULL UNIQUE,
   gender       INTEGER     NOT NULL,
   hair_style   INTEGER     NOT NULL,
   hair_color   INTEGER     NOT NULL,
   level        INTEGER     NOT NULL,
   char_pts     INTEGER     NOT NULL,
   correct_pts  INTEGER     NOT NULL,
   money        INTEGER     NOT NULL,
   x            INTEGER     NOT NULL,
   y            INTEGER     NOT NULL,
   map_id       INTEGER     NOT NULL,
   str          INTEGER     NOT NULL,
   agi          INTEGER     NOT NULL,
   dex          INTEGER     NOT NULL,
   vit          INTEGER     NOT NULL,
   int          INTEGER     NOT NULL,
   will         INTEGER     NOT NULL,
   --
   FOREIGN KEY (user_id) REFERENCES tmw_accounts(id)
);

CREATE INDEX tmw_characters_user ON tmw_characters ( user_id );
CREATE UNIQUE INDEX tmw_characters_name ON tmw_characters ( name );

CREATE TABLE tmw_char_skills 
(
    char_id     INTEGER     NOT NULL,
    skill_id    INTEGER     NOT NULL,
    skill_exp   INTEGER     NOT NULL,
    --
    FOREIGN KEY (char_id) REFERENCES tmw_characters(id)
);

CREATE INDEX tmw_char_skills_char ON tmw_char_skills ( char_id );

CREATE TABLE tmw_items 
(
    id           INTEGER    PRIMARY KEY,
    name         TEXT       NOT NULL,
    description  TEXT       NOT NULL,
    image        TEXT       NOT NULL,
    weight       INTEGER    NOT NULL,
    itemtype     TEXT       NOT NULL,
    effect       TEXT,
    dyestring    TEXT
);

CREATE INDEX tmw_items_type ON tmw_items (itemtype);

CREATE TABLE tmw_inventories 
(
   id           INTEGER     PRIMARY KEY,
   owner_id     INTEGER     NOT NULL,
   slot         INTEGER     NOT NULL,
   class_id     INTEGER     NOT NULL,
   amount       INTEGER     NOT NULL,
   --
   FOREIGN KEY (owner_id) REFERENCES tmw_characters(id)
);

CREATE TABLE tmw_guilds 
(
   id           INTEGER     PRIMARY KEY,
   name 		TEXT        NOT NULL UNIQUE
);

CREATE TABLE tmw_guild_members 
(
   guild_id     INTEGER     NOT NULL,
   member_id    INTEGER     NOT NULL,
   rights       INTEGER     NOT NULL,
   --
   FOREIGN KEY (guild_id)  REFERENCES tmw_guilds(id),
   FOREIGN KEY (member_id) REFERENCES tmw_characters(id)
);

CREATE INDEX tmw_guild_members_g ON tmw_guild_members ( guild_id );
CREATE INDEX tmw_guild_members_m ON tmw_guild_members ( member_id );

CREATE TABLE tmw_quests 
(
   owner_id     INTEGER     NOT NULL,
   name         TEXT        NOT NULL,
   value        TEXT        NOT NULL,
   --
   FOREIGN KEY (owner_id) REFERENCES tmw_characters(id)
);

CREATE TABLE tmw_world_states 
( 
   state_name   TEXT        PRIMARY KEY, 
   map_id       INTEGER     NULL, 
   value        TEXT        NULL, 
   moddate      INTEGER     NOT NULL 
);

INSERT INTO "tmw_world_states" VALUES('accountserver_startup',NULL,NULL,1221633910);
INSERT INTO "tmw_world_states" VALUES('accountserver_version',NULL,NULL,1221633910);

CREATE TABLE tmw_auctions
(
   auction_id    INTEGER     PRIMARY KEY,
   auction_state INTEGER     NOT NULL,
   char_id       INTEGER     NOT NULL,
   itemclass_id  INTEGER     NOT NULL,
   amount        INTEGER     NOT NULL,
   start_time    INTEGER     NOT NULL,
   end_time      INTEGER     NOT NULL,
   start_price   INTEGER     NOT NULL,
   min_price     INTEGER,
   buyout_price  INTEGER,
   description   TEXT,
   --
   FOREIGN KEY (char_id) REFERENCES tmw_characters(id)
);

CREATE INDEX tmw_auctions_owner ON tmw_auctions ( char_id );
CREATE INDEX tmw_auctions_state ON tmw_auctions ( auction_state );
CREATE INDEX tmw_auctions_item  ON tmw_auctions ( itemclass_id );

CREATE TABLE tmw_auction_bids
(
   bid_id        INTEGER     PRIMARY KEY,
   auction_id    INTEGER     NOT NULL,
   char_id       INTEGER     NOT NULL,
   bid_time      INTEGER     NOT NULL,
   bid_price     INTEGER     NOT NULL,
   --
   FOREIGN KEY (auction_id) REFERENCES tmw_auctions(auction_id),
   FOREIGN KEY (char_id)    REFERENCES tmw_characters(id)
);

CREATE INDEX tmw_auction_bids_auction ON tmw_auction_bids ( auction_id );
CREATE INDEX tmw_auction_bids_owner   ON tmw_auction_bids ( char_id );
