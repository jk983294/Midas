CREATE DATABASE ctp;
use ctp;

DROP TABLE IF EXISTS candle15;
CREATE TABLE candle15 (
    instrument VARCHAR(32) NOT NULL,
    date int NOT NULL,
    time int NOT NULL,
    volume double,
    open double,
    high double,
    low double,
    close double,
    PRIMARY KEY ( instrument, date, time )
) ENGINE = InnoDB DEFAULT CHARSET=utf8;

DROP TABLE IF EXISTS instrument_info;
CREATE TABLE instrument_info (
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
