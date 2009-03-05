
-- add table tmw_online_list to store online users
CREATE TABLE tmw_transactions
(
    id          INTEGER     PRIMARY KEY,
    char_id     INTEGER     NOT NULL,
    action      INTEGER     NOT NULL,
    message     TEXT,
    time        INTEGER     NOT NULL,
);
 
-- update the database version, and set date of update
UPDATE tmw_world_states
   SET value      = '3',
       moddate    = strftime('%s','now')
 WHERE state_name = 'database_version';

