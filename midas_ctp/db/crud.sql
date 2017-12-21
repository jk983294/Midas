use ctp;

-- ctp.candle15
select instrument, time, open, high, low, close, volume from ctp.candle15 order by instrument, time;
select * from ctp.candle15 where instrument like 'cu%' order by time;
delete from ctp.candle15;
insert into ctp.candle15 (instrument, time, open, high, low, close, volume)
    values (?, ?, ?, ?, ?, ?, ?);

-- ctp.instrument_info
select * from ctp.instrument_info;
select count(*) from ctp.instrument_info;
delete from ctp.instrument_info;
insert into ctp.instrument_info (InstrumentID, ExchangeID, VolumeMultiple, PriceTick, CreateDate, OpenDate, ExpireDate, LongMarginRatio, ShortMarginRatio, UnderlyingMultiple)
    values (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);
