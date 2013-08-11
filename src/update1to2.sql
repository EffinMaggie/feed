pragma journal_mode=wal;

insert or replace into option
    (otid, value)
    values
    (-1, '2')
;

delete
  from query
 where flag in ('use-proxy', 'drop-proxy');

insert into query
    (flag, parameters, query)
    values
    ('use-proxy',   1, 'insert or replace into option (otid, value) values (0, ?1)')
;

insert into query
    (flag, interval, query)
    values
    ('drop-proxy',   null, 'delete from option where otid = 0')
;
