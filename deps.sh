#!/bin/bash

# Potential dependencies for building on Debian.
# I do not guarantee that these are accurate.

sudo apt install \
  libcurl4-openssl-dev \
  libarchive-dev \
  liblzma-dev \
  rapidjson-dev \
  libacl1-dev \
  \
  make \
  g++ \
  pkg-config \
  libgl1-mesa-dev \
  libxcb*-dev \
  libfontconfig1-dev \
  libxkbcommon-x11-dev
