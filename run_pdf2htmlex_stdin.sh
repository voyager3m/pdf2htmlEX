#!/usr/bin/env bash
# example use pdf2htmlex container

docker run --rm -i -a stdin -a stdout --mount src=`pwd`,target=/pdf,type=bind pdf2htmlex $@

