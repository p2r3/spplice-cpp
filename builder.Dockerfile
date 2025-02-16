
##########
## BASE ##
##########
FROM debian:bookworm AS base

RUN apt update

# base dependencies
RUN apt install -y \
    git \
    sudo \
    wget \
    unzip \
    build-essential \
    cmake \
    libssl-dev \
    mesa-common-dev \
    libpsl-dev \
    libzstd-dev

# Found on qt5 wiki page for building from source
RUN apt install -y \
    '^libxcb.*-dev' \
    libx11-xcb-dev \
    libglu1-mesa-dev \
    libxrender-dev \
    libxi-dev \
    libxkbcommon-dev \
    libxkbcommon-x11-dev

COPY scripts/deps-packages.sh /scripts/deps-packages.sh

# From deps.sh
RUN /scripts/deps-packages.sh

##################
## DEPENDENCIES ##
##################
FROM base AS deps

WORKDIR /deps

# Copy only what's necessary to install dependencies, for docker-build caching purposes
COPY scripts/deps-packages.sh scripts/deps-packages.sh
COPY scripts/deps.sh scripts/deps.sh

RUN cd /deps/scripts; /deps/scripts/deps.sh


################
## QT5 Source ##
################
FROM deps AS qt5-source

WORKDIR /deps

COPY scripts/qt5setup.sh scripts/qt5setup.sh

RUN cd /deps/scripts; /deps/scripts/qt5setup.sh --init-only


###############
## QT5 win32 ##
###############
FROM qt5-source AS qt5-win32

WORKDIR /deps

ENV BUILD_ROOT=/deps
COPY --from=qt5-source /deps /deps
COPY scripts/qt5setup.sh scripts/qt5setup.sh

RUN cd /deps/scripts; /deps/scripts/qt5setup.sh --build-only --windows


###############
## QT5 linux ##
###############
FROM qt5-source AS qt5-linux

WORKDIR /deps

ENV BUILD_ROOT=/deps
COPY --from=qt5-source /deps /deps
COPY scripts/qt5setup.sh scripts/qt5setup.sh

RUN cd /deps/scripts; /deps/scripts/qt5setup.sh --build-only --linux


###########
## FINAL ##
###########
FROM base AS final

COPY --from=deps /deps /deps
COPY --from=qt5-win32 /deps /qt5-win32
COPY --from=qt5-linux /deps /qt5-linux
COPY docker.entrypoint.sh /docker.entrypoint.sh

WORKDIR /work

CMD ["bash", "-c", "/docker.entrypoint.sh"]
