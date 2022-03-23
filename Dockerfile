FROM ubuntu:22.04 as builder

RUN apt-get update 
RUN apt-get -y upgrade 
RUN apt-get -y install apt-utils git sudo libboost-all-dev
COPY . /usr/src/myapp
WORKDIR /usr/src/myapp
RUN ./buildScripts/makeLinuxAll


FROM ubuntu:22.04
COPY --from=builder /usr/src/myapp/imageBuild/*.deb /root/
RUN   apt update && \
  apt -y upgrade && \
  apt -y --no-install-recommends install /root/*.deb

WORKDIR /pdf



LABEL vendor="capti"

ENTRYPOINT [ "/usr/local/bin/pdf2htmlEX" ]

