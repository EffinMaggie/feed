pragma journal_mode=wal;

insert or replace into option
    (otid, value)
    values
    (-1, '2')
;

delete
  from query
 where flag in ('use-proxy', 'drop-proxy');

insert or replace into query
    (flag, parameters, query)
    values
    ('use-proxy',   1, 'insert or replace into option (otid, value) values (0, ?1)')
;

insert or replace into query
    (flag, interval, query)
    values
    ('drop-proxy',   null, 'delete from option where otid = 0'),
	('update',       null, 'update feedservice set updated = null'),
	('status',       null, 'select * from vstatus')
;

drop view if exists vstatus;
create view vstatus as
select ' [ '
    || (select count(*)
          from download
		 where completiontime is null)
    || ' / ' || count(*) || ' ]\tactive/total downloads
 [ ' || coalesce(sum(length(data)) / 1024, '-') || ' ]\ttotal download size/KiB' as description
  from download;

drop table if exists person;
create table person
(
	id integer not null primary key
);

drop table if exists personmeta;
create table personmeta
(
	id integer not null primary key,
	pid integer not null,
	mtid integer not null,
	value,

	foreign key (pid) references person(id),
	foreign key (mtid) references metatype(id)
);

drop table if exists entryperson;
create table entryperson
(
	id integer not null primary key,
	eid integer not null,
	pid integer not null,

	foreign key (eid) references entry(id),
	foreign key (pid) references person(id)
);
