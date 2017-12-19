CREATE DATABASE ctp;
use ctp;

DROP TABLE IF EXISTS ctp.candle15;
CREATE TABLE ctp.candle15 (
    instrument VARCHAR(32) NOT NULL,
    time datetime NOT NULL,
    open double,
    high double,
    low double,
    close double,
    volume double,
    PRIMARY KEY ( instrument, time )
) ENGINE = InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS ctp.instrument_info;
CREATE TABLE ctp.instrument_info (
    InstrumentID varchar(8) NOT NULL,
    ExchangeID varchar(8) NOT NULL,
    VolumeMultiple int,
    PriceTick double,
    CreateDate int,
    OpenDate int,
    ExpireDate int,
    LongMarginRatio double,
    ShortMarginRatio double,
    UnderlyingMultiple double,
    PRIMARY KEY (InstrumentID)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
