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

    unique (ibid, key, value),

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
drop table if exists icaltimezone;
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

create table icaltimezone
(
    id integer not null primary key,
    ibid integer not null,
    tzid text not null unique,
    lastmod,
    tzurl,

    foreign key (ibid) references icalblock(id)
);

-- missing sub blocks in icaltimezone:
-- standard, daylight

-- occurs in vevent, vtodo, vjournal

create table icalrecurrence
(
    uid text not null
    -- this table definition is still incomplete
);

-- automatically insert the right type of block when a new one is created

drop trigger if exists icalblockInsertEvent;
create trigger icalblockInsertEvent after insert on icalblock
for each row when new.type = 'vevent' begin
    insert or ignore into icalevent
        (ibid, uid)
        values
        (new.id, new.uid);
end;

drop trigger if exists icalblockInsertTodo;
create trigger icalblockInsertTodo after insert on icalblock
for each row when new.type = 'vtodo' begin
    insert or ignore into icaltodo
        (ibid, uid)
        values
        (new.id, new.uid);
end;

drop trigger if exists icalblockInsertJournal;
create trigger icalblockInsertJournal after insert on icalblock
for each row when new.type = 'vjournal' begin
    insert or ignore into icaljournal
        (ibid, uid)
        values
        (new.id, new.uid);
end;

drop trigger if exists icalblockInsertFreebusy;
create trigger icalblockInsertFreebusy after insert on icalblock
for each row when new.type = 'vfreebusy' begin
    insert or ignore into icalfreebusy
        (ibid, uid)
        values
        (new.id, new.uid);
end;

drop trigger if exists icalblockInsertTimezone;
create trigger icalblockInsertTimezone after insert on icalblock
for each row when new.type = 'vtimezone' begin
    insert or ignore into icaltimezone
        (ibid, tzid)
        values
        (new.id, new.uid);
end;

-- automatically fill in fields

drop trigger if exists icalattributeInsertDtstamp;
create trigger icalattributeInsertDtstamp after insert on icalproperty
for each row when new.key = 'dtstamp' begin
    update icalevent
       set dtstamp = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaltodo
       set dtstamp = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaljournal
       set dtstamp = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icalfreebusy
       set dtstamp = new.value
     where uid = (select uid from icalblock where id = new.ibid);
end;

drop trigger if exists icalattributeInsertDtstart;
create trigger icalattributeInsertDtstart after insert on icalproperty
for each row when new.key = 'dtstart' begin
    update icalevent
       set dtstart = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaltodo
       set dtstart = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaljournal
       set dtstart = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icalfreebusy
       set dtstart = new.value
     where uid = (select uid from icalblock where id = new.ibid);
end;

drop trigger if exists icalattributeInsertClass;
create trigger icalattributeInsertClass after insert on icalproperty
for each row when new.key = 'class' begin
    update icalevent
       set class = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaltodo
       set class = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaljournal
       set class = new.value
     where uid = (select uid from icalblock where id = new.ibid);
end;

drop trigger if exists icalattributeInsertCreated;
create trigger icalattributeInsertCreated after insert on icalproperty
for each row when new.key = 'created' begin
    update icalevent
       set created = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaltodo
       set created = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaljournal
       set created = new.value
     where uid = (select uid from icalblock where id = new.ibid);
end;

drop trigger if exists icalattributeInsertDescription;
create trigger icalattributeInsertDescription after insert on icalproperty
for each row when new.key = 'description' begin
    update icalevent
       set description = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaltodo
       set description = new.value
     where uid = (select uid from icalblock where id = new.ibid);
end;

drop trigger if exists icalattributeInsertGeo;
create trigger icalattributeInsertGeo after insert on icalproperty
for each row when new.key = 'geo' begin
    update icalevent
       set geo = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaltodo
       set geo = new.value
     where uid = (select uid from icalblock where id = new.ibid);
end;

drop trigger if exists icalattributeInsertLastmod;
create trigger icalattributeInsertLastmod after insert on icalproperty
for each row when new.key = 'last-mod' begin
    update icalevent
       set lastmod = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaltodo
       set lastmod = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaljournal
       set lastmod = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaltimezone
       set lastmod = new.value
     where tzid = (select uid from icalblock where id = new.ibid);
end;

drop trigger if exists icalattributeInsertLocation;
create trigger icalattributeInsertLocation after insert on icalproperty
for each row when new.key = 'location' begin
    update icalevent
       set location = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaltodo
       set location = new.value
     where uid = (select uid from icalblock where id = new.ibid);
end;

drop trigger if exists icalattributeInsertOrganizer;
create trigger icalattributeInsertOrganizer after insert on icalproperty
for each row when new.key = 'organizer' begin
    update icalevent
       set organizer = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaltodo
       set organizer = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaljournal
       set organizer = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icalfreebusy
       set organizer = new.value
     where uid = (select uid from icalblock where id = new.ibid);
end;

drop trigger if exists icalattributeInsertPriority;
create trigger icalattributeInsertPriority after insert on icalproperty
for each row when new.key = 'priority' begin
    update icalevent
       set priority = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaltodo
       set priority = new.value
     where uid = (select uid from icalblock where id = new.ibid);
end;

drop trigger if exists icalattributeInserSeq;
create trigger icalattributeInsertSeq after insert on icalproperty
for each row when new.key = 'seq' begin
    update icalevent
       set seq = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaltodo
       set seq = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaljournal
       set seq = new.value
     where uid = (select uid from icalblock where id = new.ibid);
end;

drop trigger if exists icalattributeInserStatus;
create trigger icalattributeInsertStatus after insert on icalproperty
for each row when new.key = 'status' begin
    update icalevent
       set status = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaltodo
       set status = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaljournal
       set status = new.value
     where uid = (select uid from icalblock where id = new.ibid);
end;

drop trigger if exists icalattributeInsertSummary;
create trigger icalattributeInsertSummary after insert on icalproperty
for each row when new.key = 'summary' begin
    update icalevent
       set summary = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaltodo
       set summary = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaljournal
       set summary = new.value
     where uid = (select uid from icalblock where id = new.ibid);
end;

drop trigger if exists icalattributeInsertTransp;
create trigger icalattributeInsertTransp after insert on icalproperty
for each row when new.key = 'transp' begin
    update icalevent
       set status = new.value
     where uid = (select uid from icalblock where id = new.ibid);
end;

drop trigger if exists icalattributeInsertUrl;
create trigger icalattributeInsertUrl after insert on icalproperty
for each row when new.key = 'url' begin
    update icalevent
       set url = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaltodo
       set url = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaljournal
       set url = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icalfreebusy
       set url = new.value
     where uid = (select uid from icalblock where id = new.ibid);
end;

drop trigger if exists icalattributeInsertTzurl;
create trigger icalattributeInsertTzurl after insert on icalproperty
for each row when new.key = 'tzurl' begin
    update icaltimezone
       set tzurl = new.value
     where tzid = (select uid from icalblock where id = new.ibid);
end;

drop trigger if exists icalattributeInsertRecurid;
create trigger icalattributeInsertRecurid after insert on icalproperty
for each row when new.key = 'recurid' begin
    update icalevent
       set recurid = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaltodo
       set recurid = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaljournal
       set recurid = new.value
     where uid = (select uid from icalblock where id = new.ibid);
end;

drop trigger if exists icalattributeInsertDtend;
create trigger icalattributeInsertDtend after insert on icalproperty
for each row when new.key = 'dtend' begin
    update icalevent
       set dtend = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icalfreebusy
       set dtend = new.value
     where uid = (select uid from icalblock where id = new.ibid);
end;

drop trigger if exists icalattributeInsertDuration;
create trigger icalattributeInsertDuration after insert on icalproperty
for each row when new.key = 'duration' begin
    update icalevent
       set duration = new.value
     where uid = (select uid from icalblock where id = new.ibid);
    update icaltodo
       set duration = new.value
     where uid = (select uid from icalblock where id = new.ibid);
end;

-- update feed entries for related ical block updates

drop trigger if exists icaleventupdate;
create trigger icaleventupdate after update on icalevent
for each row begin
    insert or ignore into entry
        (xid)
        values
        (new.uid);

    insert or replace into entrymeta
        (eid, mid, value)
        select entry.id as eid,
               11 as mid,
               'ical:' || new.uid as value
          from entry
         where entry.xid = new.uid;

    insert or replace into entrymeta
        (eid, mid, value)
        select entry.id as eid,
               8 as mid,
               'text/plain' as value
          from entry
         where entry.xid = new.uid;

    insert or replace into entrymeta
        (eid, mid, value)
        select entry.id as eid,
               9 as mid,
               'UTF-8' as value
          from entry
         where entry.xid = new.uid;

    insert or replace into entrymeta
        (eid, mid, value)
        select entry.id as eid,
               1 as mid,
               new.summary as value
          from entry
         where entry.xid = new.uid
           and new.summary is not null;

    insert or replace into entrymeta
        (eid, mid, value)
        select entry.id as eid,
               6 as mid,
               new.description as value
          from entry
         where entry.xid = new.uid
           and new.description is not null;

    insert or replace into entrymeta
        (eid, mid, value)
        select entry.id as eid,
               7 as mid,
               new.description as value
          from entry
         where entry.xid = new.uid
           and new.description is not null;

    insert or replace into entrymeta
        (eid, mid, value)
        select entry.id as eid,
               4 as mid,
               new.lastmod as value
          from entry
         where entry.xid = new.uid
           and new.lastmod is not null;

    insert or replace into entrymeta
        (eid, mid, value)
        select entry.id as eid,
               5 as mid,
               new.dtstamp as value
          from entry
         where entry.xid = new.uid
           and new.dtstamp is not null;

    insert or replace into entrymeta
        (eid, mid, value)
        select entry.id as eid,
               10 as mid,
               new.url as value
          from entry
         where entry.xid = new.uid
           and new.url is not null;
end;

-- sequence generator

drop view if exists vseq1;
create view vseq1  as select 0 as b union select 1 as b;
drop view if exists vseq2;
create view vseq2  as select (b1.b << 1)  | (b0.b) as b from vseq1  as b0, vseq1  as b1;
drop view if exists vseq4;
create view vseq4  as select (b1.b << 2)  | (b0.b) as b from vseq2  as b0, vseq2  as b1;
drop view if exists vseq8;
create view vseq8  as select (b1.b << 4)  | (b0.b) as b from vseq4  as b0, vseq4  as b1;
drop view if exists vseq16;
create view vseq16 as select (b1.b << 8)  | (b0.b) as b from vseq8  as b0, vseq8  as b1;
drop view if exists vseq32;
create view vseq32 as select (b1.b << 16) | (b0.b) as b from vseq16 as b0, vseq16 as b1;

drop table if exists seq1;
create table seq1
(
    b integer not null primary key
);
drop table if exists seq2;
create table seq2
(
    b integer not null primary key
);
drop table if exists seq4;
create table seq4
(
    b integer not null primary key
);
drop table if exists seq8;
create table seq8
(
    b integer not null primary key
);
drop table if exists seq16;
create table seq16
(
    b integer not null primary key
);

insert into seq1  select b from vseq1;
insert into seq2  select b from vseq2;
insert into seq4  select b from vseq4;
insert into seq8  select b from vseq8;
insert into seq16 select b from vseq16;

-- date conversions

drop table if exists jdn2weekday;
create table jdn2weekday
(
    id integer not null primary key,
    jdn double not null unique,
    weekday
);

drop trigger if exists jdn2weekdayInsert;
create trigger jdn2weekdayInsert after insert on jdn2weekday
for each row begin
    update jdn2weekday
       set weekday = cast (jdn + 0.5 as integer) % 7
     where id = new.id;
end;

drop table if exists jdn2gregorian;
create table jdn2gregorian
(
    id integer not null primary key,
    jdn double not null unique,
    year integer,
    month integer,
    day integer,
    yday integer,
    leap boolean
);

drop trigger if exists jdn2gregorianInsert;
create trigger jdn2gregorianInsert after insert on jdn2gregorian
for each row begin
    update jdn2gregorian
       set year = 1 + cast ((jdn - 1721424.5) / 365.2425 as integer)
     where id = new.id;

    update jdn2gregorian
       set yday = 365
                - cast (365.2425 * year as integer)
                + cast (jdn - 1721424.5 as integer)
     where id = new.id;

    update jdn2gregorian
       set yday = 366,
           year = year - 1
     where id = new.id
       and yday = 0
       and (((year - 1) % 4 = 0) and not (((year - 1) % 100 = 0) or ((year - 1) % 400 = 0)));

    update jdn2gregorian
       set yday = 1
     where id = new.id
       and yday = 0;

    update jdn2gregorian
       set leap = ((year % 4 = 0) and not ((year % 100 = 0) or (year % 400 = 0)))
     where id = new.id;
end;
