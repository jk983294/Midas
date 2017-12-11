#!/bin/bash

node1 -a 8010 -d 8020 --heart_beat_interval 10
node2 -a 8011 -d 8021 -s 8020
node3 -a 8012 -s 8021
