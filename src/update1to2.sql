pragma journal_mode=wal;

-- update schema version number
insert or replace into option
    (otid, value)
    values
    (-1, '2'),
    ( 1, 'FEED/2')
;

-- tell old daemons to shut down
insert or ignore into clientcommand
    (cmid)
    values
    (2)
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

update relation
   set description = 'links to'
 where id = 2;

insert or replace into relation
    (id, description)
    values
    ( 4, 'is author of'),
    ( 5, 'is contributor to'),
    ( 6, 'is editor of'),
    ( 7, 'is webmaster for'),
    ( 8, 'encloses')
;

drop view if exists vstatus;
create view vstatus as
select ' [ '
    || (select count(*)
          from download
         where completiontime is null)
    || ' / ' || count(*) || ' ]\tactive/total downloads
 [ ' || coalesce(sum(length(data)) / 1024, '-') || ' ]\ttotal download size/KiB' as description
  from download
union
select ' [ ' || group_concat(pid) || ' ]\tdaemon PID' as description
  from instance;

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

    unique (pid, mtid, value),

    foreign key (pid) references person(id),
    foreign key (mtid) references metatype(id)
);

drop table if exists entryperson;
create table entryperson
(
    id integer not null primary key,
    eid integer not null,
    pid integer not null,
    rid integer not null,

    foreign key (eid) references entry(id),
    foreign key (pid) references person(id),
    foreign key (rid) references relation(id)
);

drop view if exists vperson;
create view vperson as
select person.id   as pid,
       name.value  as name,
       email.value as email,
       uri.value   as uri
  from person
  left join personmeta as name  on name.mtid  = 12 and name.pid  = person.id
  left join personmeta as email on email.mtid = 13 and email.pid = person.id
  left join personmeta as uri   on uri.mtid   = 10 and uri.pid   = person.id;

drop trigger if exists feedDelete;
create trigger feedDelete after delete on feed
for each row begin
    delete
      from feedservice
     where fid = old.id;
end;
