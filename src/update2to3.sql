-- update schema version number
insert or replace into option
    (otid, value)
    values
    (-1, '3')
;

insert or replace into service
    (id, description, interval, download, bound, enabled)
    values
    ( 4, 'iCal reader', 0.1, 1, 1, 1)
;
