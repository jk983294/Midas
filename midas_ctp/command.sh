#!/bin/bash

cp -r cfg ~/Data/ctp

midas -a 8023 -C /home/kun/Data/ctp/cfg/midas.info

# check all product code and name
awk '{print $2, $4}' product.dump

# get all instrument with code / exchange / expire date
awk '{print $2, $4, $34}' instrument.dump
