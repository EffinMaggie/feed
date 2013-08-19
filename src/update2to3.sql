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
