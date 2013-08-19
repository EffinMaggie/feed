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

-- iCal tables and views

drop table if exists icalblock;
drop table if exists icalproperty;
drop table if exists icalattribute;

create table icalblock
(
    id integer not null primary key,
    uid text not null unique,
    type text not null,
    ibid integer null,

    foreign key (ibid) references icalblock(id)
);

create table icalproperty
(
    id integer not null primary key,
    key text not null,
    value text,

    ibid integer not null,

    unique (ibid, key, value)

    foreign key (ibid) references icalblock(id)
);

create table icalattribute
(
    id integer not null primary key,
    key text not null,
    value text not null,

    ipid integer not null,

    unique (ipid, key, value),

    foreign key (ipid) references icalproperty(id)
);

-- specific iCal block tables

drop table if exists icalbody;
drop table if exists icalevent;
drop table if exists icaltodo;
drop table if exists icaljournal;
drop table if exists icalfreebusy;
drop table if exists icalrecurrence;

create table icalbody
(
    id integer not null primary key,
    ibid integer not null,
    uid text not null unique,
    prodid text,
    version,
    calscale,
    method,

    foreign key (ibid) references icalblock(id)
);

create table icalevent
(
    id integer not null primary key,
    ibid integer not null,
    uid text not null unique,
    dtstamp numeric,
    dtstart numeric,
    class,
    created,
    description,
    geo,
    lastmod,
    location,
    organizer,
    priority,
    seq,
    status,
    summary,
    transp,
    url,
    recurid,
    dtend,
    duration,

    foreign key (ibid) references icalblock(id)
);

create trigger icalblockInsertEvent after insert on icalblock
for each row when new.type = 'vevent' begin
    insert or ignore into icalevent
        (ibid, uid)
        values
        (new.id, new.uid);
end;

-- missing fields that may occur an arbitrary number of times in vevents:
-- attach, attendee, categories, comment, contact, exdate, rstatus, related,
-- resources, rdate

create table icaltodo
(
    id integer not null primary key,
    ibid integer not null,
    uid text not null unique,
    dtstamp numeric,
    class,
    completed,
    created,
    description,
    dtstart numeric,
    geo,
    lastmod,
    location,
    organizer,
    percent,
    priority,
    recurid,
    seq,
    status,
    summary,
    url,
    due,
    duration,

    foreign key (ibid) references icalblock(id)
);

-- missing fields that may occur an arbitrary number of times in vtodos:
-- attach, attendee, categories, comment, contact, exdate, rstatus, related,
-- resources, rdate

create table icaljournal
(
    id integer not null primary key,
    ibid integer not null,
    uid text not null unique,
    dtstamp numeric,
    class,
    created,
    dtstart,
    lastmod,
    organizer,
    recurid,
    seq,
    status,
    summary,
    url,

    foreign key (ibid) references icalblock(id)
);

-- missing fields that may occur an arbitrary number of times in vjournals:
-- attach, attendee, categories, comment, contact, description, exdate,
-- related, rdate, rstatus

create table icalfreebusy
(
    id integer not null primary key,
    ibid integer not null,
    uid text not null unique,
    dtstamp numeric,
    contact,
    dtstart numeric,
    dtend numeric,
    organizer,
    url,

    foreign key (ibid) references icalblock(id)
);

-- missing fields that may occur an arbitrary number of times in vfreebusy:
-- attendee, comment, freebusy, rstatus

-- occurs in vevent, vtodo, vjournal

create table icalrecurrence
(
    uid text not null
    -- this table definition is still incomplete
);

-- feed-graph views

drop view if exists vgraphnodes;
create view vgraphnodes as
select 'ne' || id as nid,
       0.1 as margin,
       replace(replace(coalesce ((select value from entrymeta where mid=1 and eid=entry.id), 'entry #' || id), '"', ''''), '
', '') as title,
       replace(replace((select value from entrymeta where mid=10 and eid=entry.id), '"', ''''), '
', '') as uri
  from entry
 where exists (select value from entrymeta where mid=11 and eid=entry.id)
    or exists (select id from feed where source = (select value from entrymeta where mid=10 and eid=entry.id))
union
select 'np' || id as nid,
       0.2 as margin,
       (select coalesce(name,email,uri,'person #' || pid) from vperson where pid=person.id) as title,
       (select coalesce(uri,'mailto:' || email,null) from vperson where pid=person.id) as uri
  from person;

drop view if exists vgraphedges;
create view vgraphedges as
select distinct
       'ne' || eid1 as nid1,
       'ne' || eid2 as nid2,
       'color="blue" weight=1 len=3' as attributes
 from entryrelation where eid1 <> eid2
union
select distinct
       'np' || pid as nid1,
       'ne' || eid as nid2,
       'color="red" weight=1.5 len=4' as attributes
  from entryperson
union
select distinct
       'ne' || (select eid from entrymeta as m2 where mid=10 and m1.value = m2.value) as nid1,
       'ne' || eid as nid2,
       'color="purple" weight=4 len=5' as attributes
  from entrymeta as m1
 where mid=11
;

drop view if exists vegraphedges;
create view vegraphedges as
select distinct
       nid1, nid2, attributes
  from vgraphedges, vgraphnodes as n1, vgraphnodes as n2
 where vgraphedges.nid1 = n1.nid
   and vgraphedges.nid2 = n2.nid
;
