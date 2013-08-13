create table optiontype
(
    id integer not null primary key,
    description text not null
);

insert into optiontype
    (id, description)
    values
    (-1, "Schema Version"),
    ( 0, "Proxy server URI"),
    ( 1, "HTTP user agent")
;

create table option
(
    id integer not null primary key,
    otid integer not null unique,
    value,

    foreign key (otid) references optiontype(id)
);

insert into option
    (otid, value)
    values
    (  -1, '2');

create table service
(
    id integer primary key,
    description text,
    interval float,
    download boolean default 0,
    bound boolean default 1,
    enabled boolean default 0
);

create table feed
(
    id integer primary key,
    sid integer null,
    source text not null unique,

    foreign key (sid) references service(id)
);

insert into service
    (id, description, interval, download, bound, enabled)
    values
    ( 0, 'Atom feed reader', 0.1, 1, 1, 1),
    ( 1, 'RSS feed reader', 0.1, 1, 1, 1),
    ( 2, 'Whois lookup', 0.2, 0, 1, 1),
    ( 3, 'DNS lookup', 0.05, 0, 1, 1),
    ( 5, 'XHTML feed gatherer', 0.1, 1, 1, 1),
    ( 6, 'HTML feed gatherer', 0.1, 1, 1, 1),
    ( 7, 'Download', 0.05, 0, 1, 1)
;

create table feedservice
(
    fid integer not null,
    sid integer not null,
    updated float null,

    primary key (fid, sid),
    foreign key (fid) references feed(id),
    foreign key (sid) references service(id)
);

create trigger feedInsert after insert on feed
for each row when new.sid is null
begin
    insert into feedservice
        (fid, sid, updated)
        select new.id as fid,
               service.id sid,
               null as updated
          from service
         where bound;
end;

create trigger feedInsertSpecific after insert on feed
for each row when new.sid is not null
begin
    insert into feedservice
        (fid, sid, updated)
        values
        (new.id, new.sid, null);
end;

create view vfeedupdate as
select
  feed.id as fid,
  feed.source,
  service.id as sid,
  coalesce(julianday(feedservice.updated) + julianday(service.interval),
           julianday('now')) as nextupdate
from feedservice, feed, service
where feedservice.fid = feed.id
  and feedservice.sid = service.id
  and service.enabled;

create table entry
(
    id integer primary key,
    xid text not null unique,
    updated float null
);

create table meta
(
    id integer primary key,
    description text
);

insert into meta
    (id, description)
    values
    ( 0, 'content ID'),
    ( 1, 'title'),
    ( 2, 'subtitle'),
    ( 3, 'language'),
    ( 4, 'date and time of last update'),
    ( 5, 'date and time when the entry was published'),
    ( 6, 'abstract'),
    ( 7, 'content'),
    ( 8, 'content MIME type'),
    ( 9, 'content encoding'),
    (10, 'canonical URI'),
    (11, 'source feed'),
    (12, 'author name'),
    (13, 'author email'),
    (14, 'marked as read'),
    (15, 'marked')
;

create table entrymeta
(
    id integer not null primary key,
    eid integer not null,
    mid integer not null,
    value not null,

    unique (eid, mid),

    foreign key (eid) references entry(id),
    foreign key (mid) references meta(id)
);

create trigger entrymetaFixMarker15 after insert on entrymeta
for each row when new.mid = 15 begin
    delete
      from entrymeta
     where id <> new.id
       and mid = 15;
end;

create trigger entrymetaFixMarker14 after insert on entrymeta
for each row when new.mid = 14 begin
    delete
      from entrymeta
     where eid = new.eid
       and mid = 15;
end;

create trigger entrymetaFixTime after insert on entrymeta
for each row when new.mid in (4,5) and julianday(new.value) is not null and julianday(new.value) <> new.value begin
    update entrymeta
       set value = julianday(new.value)
     where id = new.id;
end;

create trigger entrymetaFixTimeRFC822 after insert on entrymeta
for each row when new.mid in (4,5) and julianday(new.value) is null and (length(new.value) >= 26) begin
    update entrymeta
       set value
         = julianday(substr(new.value,13,4)
           || '-' || case substr(new.value,9,3)
                     when 'Jan' then '01'
                     when 'Feb' then '02'
                     when 'Mar' then '03'
                     when 'Apr' then '04'
                     when 'May' then '05'
                     when 'Jun' then '06'
                     when 'Jul' then '07'
                     when 'Aug' then '08'
                     when 'Sep' then '09'
                     when 'Oct' then '10'
                     when 'Nov' then '11'
                     when 'Dec' then '12'
                     end
           || '-' || substr(new.value,6,2)
           || 'T' || substr(new.value,18,8))
     where id = new.id;
end;

create table relation
(
    id integer primary key,
    description text
);

insert into relation
    (id, description)
    values
    ( 0, 'is alternate version of'),
    ( 1, 'uses'),
    ( 2, 'links to'),
    ( 3, 'is payment link of'),
    ( 4, 'is author of'),
    ( 5, 'is contributor to'),
    ( 6, 'is editor of'),
    ( 7, 'is webmaster for'),
    ( 8, 'encloses')
;

create table entryrelation
(
    eid1 integer not null,
    eid2 integer not null,
    rid integer not null,

    primary key (eid1, eid2, rid),

    foreign key (eid1) references entry(id),
    foreign key (eid2) references entry(id),
    foreign key (rid) references relation(id)
);

create trigger entryrelationSync after insert on entryrelation
for each row begin
    insert or ignore into entrymeta
        (eid, mid, value)
        select new.eid2 as eid,
               mid,
               value
          from entrymeta
         where eid = new.eid1;

    insert or ignore into entrymeta
        (eid, mid, value)
        select new.eid1 as eid,
               mid,
               value
          from entrymeta
         where eid = new.eid2;
end;

create table status
(
    id integer primary key,
    description text
);

insert into status
    (id, description)
    values
    ( 0, 'working'),
    ( 1, 'complete'),
    ( 2, 'failure')
;

create table download
(
    id integer primary key,
    uri text not null,
    requesttime float not null,
    payload text,
    sid integer,
    data blob,
    length integer,
    completiontime float,

    foreign key (sid) references service(id)
);

create trigger downloadUpdate after update on download
for each row when new.completiontime is not null and old.completiontime is null begin
    update feedservice
       set updated = null
     where fid in (select feed.id
                     from feed
                    where feed.source = new.uri)
       and sid in (select service.id
                     from service
                    where service.download);
end;

create table whoisdownload
(
    id integer not null primary key,
    fid integer not null,
    uri text,
    payload text,
    query text,

    foreign key (fid) references feed(id),
    foreign key (uri, payload) references download (uri, payload)
);

create table whois
(
    id integer not null primary key,
    query text not null unique
);

create table whoisdetail
(
    id integer not null primary key,
    wid integer not null,
    line integer not null,
    key text not null,
    value text not null,

    unique (wid, line, key),

    foreign key (wid) references whois(id)
);

create table protocol
(
    id integer not null primary key,
    description text
);

insert into protocol
    (id, description)
    values
    ( 0, 'IPv4'),
    ( 1, 'IPv6')
;

create table dns
(
    id integer not null primary key,
    hostname text not null,
    pid integer not null,
    address text not null,

    unique (hostname, pid, address),

    foreign key (pid) references protocol(id)
);

create trigger dnsWhois after insert on dns
for each row begin
    insert into feed
        (sid, source)
        values
        (2, 'whois://whois.arin.net/#' || new.address);
end;

create table command
(
    id integer primary key,
    description text
);

insert into command
    (id, description)
    values
    ( 0, 'update feed'),
    ( 1, 'query'),
    ( 2, 'quit'),
    ( 3, 'download complete')
;

create table query
(
    id integer primary key,
    flag text null unique,
    accelerator text null,
    query text not null,
    interval float null,
    updated float null,
    parameters integer not null default 0,
    shell integer null,
    help text null,

    check (flag is not null or accelerator is not null)
);

create view vquery as
select id as qid, flag, accelerator, query, interval, updated,
       coalesce(updated + interval, julianday('now')) as nextupdate
  from query
 where interval is not null
   and parameters = 0;

create table clientcommand
(
    cmid integer not null,
    fid integer null,
    sid integer null,
    qid integer null,
    did integer null,

    foreign key (cmid) references command(id),
    foreign key (fid) references feed(id),
    foreign key (sid) references service(id),
    foreign key (qid) references query(id),
    foreign key (did) references download(id)
);

create trigger downloadCompleteNotification after update on download
for each row when new.completiontime is not null and old.completiontime is null and new.sid is not null begin
    insert into clientcommand
        (cmid, sid, did)
        values
        (   3, new.sid, new.id);
end;

create view ventries as
select id
  from entry;

insert into query
    (flag, interval, query)
    values
    ('list',         null, 'select ''['' || eid || '']'', round(updated,2), title from vheadline order by updated asc'),
    ('list-feeds',   null, 'select ''['' || id || '']'', source from feed'),
    ('clean',        0.05, 'delete from download where id < (select max (id) from download as d2 where download.uri = d2.uri and download.payload is d2.payload)'),
    ('purge',        null, 'delete from download where completiontime'),
    ('new',          null, 'select ''['' || eid || '']'', round(updated,2), title from vheadline where read = 0 order by updated asc'),
    ('quit',         null, 'insert or ignore into clientcommand (cmid) values (2)'),
    ('time',         null, 'select julianday(''now'')'),
    ('mark-next',    null, 'insert or ignore into entrymeta (eid, mid, value) select eid, 15, 1 from vheadline where read = 0 order by updated desc limit 1'),
    ('mark-read',    null, 'insert or ignore into entrymeta (eid, mid, value) select eid, 14, 1 from entrymeta where mid = 15 limit 1'),
    ('drop-proxy',   null, 'delete from option where otid = 0'),
    ('title-marked', null, 'select ''['' || eid || '']'', round(updated,2), title from vheadline where marked order by updated desc limit 1'),
    ('all-read',     null, 'insert or ignore into entrymeta (eid, mid, value) select id, 14, 1 from entry'),
	('update',       null, 'update feedservice set updated = null'),
	('status',       null, 'select * from vstatus')
;

insert into query
    (flag, parameters, query)
    values
    ('add',         1, 'insert into feed (source) values (?1)'),
    ('remove',      1, 'delete from feed where source = ?1'),
    ('use-proxy',   1, 'insert or replace into option (otid, value) values (0, ?1)')
;

create table viewer
(
    id integer not null primary key,
    mime text not null,
    command text not null,
    priority integer not null default 10
);

insert into viewer
    (mime, command, priority)
    values
    ('image/%', 'cacaview ${filename}',           40),
    ('audio/%', 'aplay ${filename}',              40),
    ('%html%',  'links -force-html ${filename}',  50),
    ('%',       'less ${filename}',              100)
;

create view ventrydata as
select id as eid,
       coalesce((select value from entrymeta where eid = entry.id and mid = 7),
                (select value from entrymeta where eid = entry.id and mid = 6),
                (select value from entrymeta where eid = entry.id and mid = 1),
                'no content') as data,
       coalesce((select value from entrymeta where eid = entry.id and mid = 8
                                               and exists (select eid from entrymeta where eid = entry.id and mid = 7)),
                'text/plain') as mime
  from entry;

create view ventryviewer as
select eid,
       data,
       ventrydata.mime,
       command
  from ventrydata,
       viewer
 where ventrydata.mime like viewer.mime
 order by eid, priority;

insert into query
    (flag, shell, parameters, query)
    values
    ('read',        1, 1, 'select command, data from ventryviewer where eid = ?1 limit 1'),
    ('read-marked', 1, 0, 'select command, data from ventryviewer, entrymeta where ventryviewer.eid = entrymeta.eid and mid = 15 and value = 1')
;

create table querymacro
(
    id integer not null primary key,
    line integer not null default 5,
    macro text not null,
    flag text not null,
    help text null,

    foreign key (macro) references query(flag),
    foreign key (flag) references query(flag)
);

insert into querymacro
    (line, macro, flag)
    values
    (   1, 'next', 'mark-next'),
    (   2, 'next', 'read-marked'),
    (   3, 'next', 'mark-read'),
    (   4, 'next', 'mark-next'),
    (   5, 'next', 'title-marked'),

    (   1, 'skip', 'mark-next'),
    (   2, 'skip', 'mark-read'),
    (   3, 'skip', 'mark-next'),
    (   4, 'skip', 'title-marked')
;

create view vcommand as
select 5 as priority,
       cmid,
       fid,
       sid,
       qid,
       did
  from clientcommand
 union select
       10 as priority,
       0 as cmid,
       fid,
       sid,
       null as qid,
       null as did
  from vfeedupdate
 where nextupdate <= julianday('now')
 union select
       20 as priority,
       1 as cmid,
       null as fid,
       null as sid,
       qid as qid,
       null as did
  from vquery
 where nextupdate <= julianday('now');

create view vheadline as
select entry.id as eid,
       entry.xid as xid,
       (select value from entrymeta where entrymeta.eid = entry.id and entrymeta.mid = 0) as cid,
       coalesce((select value from entrymeta where entrymeta.eid = entry.id and entrymeta.mid = 1), 'no title') as title,
       coalesce(julianday((select value from entrymeta where entrymeta.eid = entry.id and entrymeta.mid = 4)),
                julianday((select value from entrymeta where entrymeta.eid = entry.id and entrymeta.mid = 5))) as updated,
       (select value from entrymeta where entrymeta.eid = entry.id and entrymeta.mid = 5) as published,
       (select value from entrymeta where entrymeta.eid = entry.id and entrymeta.mid = 14) is not null as read,
       (select value from entrymeta where entrymeta.eid = entry.id and entrymeta.mid = 15) is not null as marked
  from entry
;

create table instance
(
    id integer not null primary key,
    pid integer not null unique
);

-- options

insert or replace into option
    (otid, value)
    values
    -- The User Agent string to send
    (   1, 'FEED/2')
;

-- new tables and views from version 2

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
