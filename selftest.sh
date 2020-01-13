#!/bin/bash

# a simple test
if [ "$(echo hello | ./wc2a)" != "$(echo hello | wc)" ]; then
  echo "fail"
  exit 1
fi

# a more complex test
if [ "$(./wc2a wc2a.c)" != "$(wc wc2a.c)" ]; then
  echo "fail"
  exit 1
fi


echo "success"
exit 0
