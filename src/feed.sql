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
    (  -1, '1');

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
    (11, 'source feed')
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
    ( 2, 'is hyperlink to'),
    ( 3, 'is payment link of')
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
    ( 2, 'failure');

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
    flag text null,
    accelerator text null,
    query text not null,
    interval float null,
    updated float null,
    parameters integer not null default 0,
    shell integer null,

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
    (id, flag, interval, shell, query)
    values
    ( 1, 'list',   null, 0, 'select eid, updated, title from vheadline order by updated asc'),
    ( 2, 'clean',  0.05, 0, 'delete from download where id < (select max (id) from download as d2 where download.uri = d2.uri and download.payload is d2.payload)'),
    ( 3, 'purge',  null, 0, 'delete from download where completiontime'),
    ( 4, 'new',    null, 0, 'select eid, updated, title from vheadline where read = 0 order by updated asc')
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

create table log
(
    cmid integer null,
    fid integer null,
    sid integer null,
    qid integer null,
    did integer null,
    eventtime float not null,
    priority integer not null,
    message text not null,

    foreign key (cmid) references command(id),
    foreign key (fid) references feed(id),
    foreign key (sid) references service(id),
    foreign key (qid) references query(id),
    foreign key (did) references download(id)
);

create view vlog as
select cmid,
       fid,
       sid,
       qid,
       did,
       eventtime,
       priority,
       message
  from log;

create trigger vlogInsert instead of insert on vlog
for each row
begin
  insert into log
      (cmid, fid, sid, qid, did, eventtime, priority, message)
      values
      (new.cmid, new.fid, new.sid, new.qid, new.did,
       coalesce(new.eventtime, julianday('now')),
       coalesce(new.priority, 5),
       new.message);
end;

create view vheadline as
select entry.id as eid,
       entry.xid as xid,
       (select value from entrymeta where entrymeta.eid = entry.id and entrymeta.mid = 0) as cid,
       coalesce((select value from entrymeta where entrymeta.eid = entry.id and entrymeta.mid = 1), 'no title') as title,
       coalesce(julianday((select value from entrymeta where entrymeta.eid = entry.id and entrymeta.mid = 4)),
                julianday((select value from entrymeta where entrymeta.eid = entry.id and entrymeta.mid = 5))) as updated,
       (select value from entrymeta where entrymeta.eid = entry.id and entrymeta.mid = 5) as published
  from entry
;

create table headline
(
    id integer not null primary key,
    eid integer not null,
    xid integer,
    cid text,
    title text
);

create table instance
(
    id integer not null primary key,
    pid integer not null unique
);

-- options

insert or replace into option
    (otid, value)
    values
    -- CURL proxy settings
    (   0, 'socks5h://127.0.0.1:12050'),

    -- The User Agent string to send
    (   1, 'FEED/0')
;

insert into feed
    (source)
    values
    ('http://ef.gy/'),
    ('http://xkcd.com/'),
    ('http://what-if.xkcd.com/'),
    ('http://thedailywtf.com/'),
    ('http://git.becquerel.org/?p=jyujin/feed.git;a=atom')
;
