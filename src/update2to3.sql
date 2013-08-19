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
