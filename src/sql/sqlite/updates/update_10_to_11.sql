--
-- SQLite does not support removing of columns, so we'll just let them be
--

--ALTER TABLE `mana_characters` DROP `money`;
--ALTER TABLE `mana_characters` DROP `str`;
--ALTER TABLE `mana_characters` DROP `agi`;
--ALTER TABLE `mana_characters` DROP `vit`;
--ALTER TABLE `mana_characters` DROP `int`;
--ALTER TABLE `mana_characters` DROP `dex`;
--ALTER TABLE `mana_characters` DROP `will`;

CREATE TABLE mana_char_attr
(
   char_id      INTEGER     NOT NULL,
   attr_id      INTEGER     NOT NULL,
   attr_base    FLOAT       NOT NULL,
   attr_mod     FLOAT       NOT NULL,
   --
   FOREIGN KEY (char_id) REFERENCES mana_characters(id)
);

CREATE INDEX mana_char_attr_char ON mana_char_attr ( char_id );

CREATE TABLE mana_char_equips
(
    id               INTEGER    PRIMARY KEY,
    owner_id         INTEGER    NOT NULL,
    slot_type        INTEGER    NOT NULL,
    inventory_slot   INTEGER    NOT NULL,
    --
    FOREIGN KEY (owner_id) REFERENCES mana_characters(id)
);

-- update the database version, and set date of update
UPDATE mana_world_states
   SET value      = '11',
       moddate    = strftime('%s','now')
 WHERE state_name = 'database_version';
