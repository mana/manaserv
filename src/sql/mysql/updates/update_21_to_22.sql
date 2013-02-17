BEGIN;

-- Empty update since we do not want to destroy your skill data.
-- There is no way to convert all your skills to attributes. You will have to
-- do this manually.

-- Update database version.
UPDATE mana_world_states
    SET value = '22',
        moddate = UNIX_TIMESTAMP()
    WHERE state_name = 'database_version';

END;
