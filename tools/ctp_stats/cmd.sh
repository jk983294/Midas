#!/bin/bash

dp -s 20171112.000000 -e 20171112.160000 | ctp_stats > result
ctp_stats -t 1 -s /home/kun/github/midas/midas_ctp/cfg/trading_hour.map