#!/usr/bin/env bash
# example use pdf2htmlex container

#docker run --rm -it --mount src=`pwd`,target=/pdf,type=bind pdf2htmlex -f 1 -l 2 ./WHO-2019-nCoV-laboratory-2020.6-eng.pdf 

docker run --rm -it --mount src=`pwd`,target=/pdf,type=bind pdf2htmlex $@

