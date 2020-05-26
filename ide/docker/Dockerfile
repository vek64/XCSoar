# docker build -f docker/Dockerfile . -t xc-wince-build

# sudo docker run -v "$(pwd)"/XCSoar:/home/yocto/XCSoar -it xc-wince-build


FROM ubuntu:18.04

ENV DEBIAN_FRONTEND=noninteractive

RUN dpkg --add-architecture i386
RUN apt-get update
RUN apt-get install -y g++-5-multilib
RUN apt-get install -y curl dosfstools gawk g++-multilib gcc-multilib lib32z1-dev libcrypto++6:i386 libcrypto++-dev:i386 liblzo2-dev:i386 lzop libsdl1.2-dev libstdc++-5-dev:i386 libusb-1.0-0:i386 libusb-1.0-0-dev:i386 uuid-dev:i386 texinfo chrpath

RUN apt-get install -y cpio diffstat wget

RUN apt-get install -y python git

RUN apt-get install -y locales vim quilt imagemagick ffmpeg mtools parted sudo

RUN apt-get install -y libcurl4-openssl-dev pkg-config awscli libboost-all-dev librsvg2-bin xsltproc imagemagick gettext zlib1g-dev libsdl1.2-dev libfreetype6-dev libpng-dev libjpeg-dev libcurl4-openssl-dev libxml-parser-perl librsvg2-bin xsltproc imagemagick gettext fonts-dejavu packaging-dev ccache libfl-dev

RUN apt-get clean


ADD http://max.kellermann.name/download/xcsoar/devel/cegcc/mingw32ce-mk-2020-03-23-amd64.tar.xz .
RUN tar xvf mingw32ce-mk-2020-03-23-amd64.tar.xz -C /opt/ \
 && rm mingw32ce-mk-2020-03-23-amd64.tar.xz \
 && ln -s /opt/mingw32ce-mk-2020-03-23-amd64 /opt/mingw32ce
RUN cd ~



RUN useradd -ms /bin/bash  yocto && \
    /usr/sbin/locale-gen en_US.UTF-8
RUN adduser yocto sudo
RUN echo 'yocto:blub' | chpasswd

USER yocto

RUN mkdir ~/bin
RUN curl http://commondatastorage.googleapis.com/git-repo-downloads/repo > ~/bin/repo
RUN chmod a+x ~/bin/repo

ENV LC_ALL=en_US.UTF-8
ENV LANG=en_US.UTF-8

ENV PATH="~/bin:${PATH}"
ENV PATH="$PATH:/opt/mingw32ce/bin"

WORKDIR /home/yocto/
CMD /bin/bash

