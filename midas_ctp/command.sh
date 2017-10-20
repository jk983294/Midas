#!/bin/bash

cp -r cfg ~/Data/ctp

midas -a 8023 -C /home/kun/Data/ctp/cfg/midas.info -L debug

# admin generic
admin 0:8023 net_help
admin 0:8023 meters

# query local data
admin 0:8023 query book cu1712
admin 0:8023 query image cu1712
admin 0:8023 query instrument
admin 0:8023 query instrument cu1712
admin 0:8023 query product
admin 0:8023 query product cu
admin 0:8023 query exchange
admin 0:8023 query exchange SHFE
admin 0:8023 query account
admin 0:8023 query position

# dump local data
admin 0:8023 dump
admin 0:8023 dump all
admin 0:8023 dump instrument
admin 0:8023 dump product
admin 0:8023 dump exchange
admin 0:8023 dump account
admin 0:8023 dump position

# request remote data
admin 0:8023 request
admin 0:8023 request all
admin 0:8023 request instrument
admin 0:8023 request product
admin 0:8023 request exchange
admin 0:8023 request account
admin 0:8023 request position

# get <exchange, name>
awk '{print $2, $4}' exchange.dump | sort > exchange.info

# get <exchange, product_code, name>
awk '{print $6, $2, $4}' product.dump | sort > product.info

# get all instrument with code / exchange / expire date
awk '{print $4, $2, $34}' instrument.dump
