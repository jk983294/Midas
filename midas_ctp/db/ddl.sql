CREATE DATABASE ctp;
use ctp;

CREATE TABLE candle15 (
    instrument VARCHAR(32) NOT NULL,
    date INT NOT NULL,
    time INT NOT NULL,
    volume DOUBLE,
    open DOUBLE,
    high DOUBLE,
    low DOUBLE,
    close DOUBLE,
    PRIMARY KEY ( instrument, date, time )
) ENGINE = InnoDB DEFAULT CHARSET=utf8;
