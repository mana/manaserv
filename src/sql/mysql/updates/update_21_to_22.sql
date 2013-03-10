START TRANSACTION;

ALTER TABLE mana_inventories ADD COLUMN equipped tinyint(3) unsigned NOT NULL;

INSERT INTO mana_inventories (owner_id, slot, class_id, amount, equipped)
    SELECT owner_id, (SELECT MAX(slot) + 1 FROM mana_inventories
        WHERE owner_id=owner_id),
    item_id, 1, 1 FROM mana_char_equips;

DROP TABLE mana_char_equips;

-- Update database version.
UPDATE mana_world_states
    SET value = '22',
        moddate = UNIX_TIMESTAMP()
    WHERE state_name = 'database_version';

COMMIT;
