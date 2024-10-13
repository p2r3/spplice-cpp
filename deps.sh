# Builds statically linkable dependencies for both supported platforms
# On the Windows side, most of this is just unzipping and sorting pre-built binaries
# On the Linux side, we're only compiling CURL and installing Debian packages

rm -rf ./deps
mkdir deps
cd deps

mkdir win32
mkdir linux
mkdir shared

# Build shared dependencies
  cd shared

    wget https://duktape.org/duktape-2.7.0.tar.xz
    tar -xf ./duktape-2.7.0.tar.xz
    rm ./duktape-2.7.0.tar.xz
    mv ./duktape-2.7.0/src ./duktape
    rm -rf duktape-2.7.0

    wget https://github.com/upx/upx/releases/download/v4.2.4/upx-4.2.4-amd64_linux.tar.xz
    tar -xf ./upx-4.2.4-amd64_linux.tar.xz
    rm ./upx-4.2.4-amd64_linux.tar.xz
    mv ./upx-4.2.4-amd64_linux ./upx

  cd ..

# Build Windows dependencies
  cd win32

    mkdir bin
    mkdir lib
    mkdir include

    mkdir xz; cd xz
      wget https://tukaani.org/xz/xz-5.2.9-windows.zip
      unzip xz-5.2.9-windows.zip
      mv ./bin_x86-64/*.exe ../bin
      mv ./bin_x86-64/* ../lib
    cd ..; rm -rf xz

    mkdir archive; cd archive
      wget https://libarchive.org/downloads/libarchive-v3.7.4-amd64.zip
      unzip libarchive-v3.7.4-amd64.zip
      mv ./libarchive/bin/*.exe ../bin
      mv ./libarchive/bin/*.dll ../lib
      mv ./libarchive/lib/* ../lib
      mv ./libarchive/include/* ../include
    cd ..; rm -rf archive

    mkdir openssl; cd openssl
      wget https://wiki.overbyte.eu/arch/openssl-1.1.1w-win64.zip
      unzip openssl-1.1.1w-win64.zip
      mv ./*.exe ../bin
      mv ./*.dll ../lib
    cd ..; rm -rf openssl

    mkdir curl; cd curl
      wget https://curl.se/windows/dl-8.10.1_2/curl-8.10.1_2-win64-mingw.zip
      unzip curl-8.10.1_2-win64-mingw.zip
      mv curl-8.10.1_2-win64-mingw curl
      mv ./curl/bin/*.exe ../bin
      mv ./curl/bin/*.dll ../lib
      mv ./curl/lib/* ../lib
      mv ./curl/include/* ../include
    cd ..; rm -rf curl

  cd ..

# Build Linux dependencies
  cd linux

    mkdir bin
    mkdir lib
    mkdir include

    mkdir curl; cd curl
      wget https://curl.se/download/curl-8.10.1.zip
      unzip curl-8.10.1.zip
      mv curl-8.10.1 curl; cd curl
        ./configure --enable-optimize --with-openssl --disable-dependency-tracking --disable-shared --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --disable-mqtt --disable-manual --disable-docs --disable-sspi --disable-aws --disable-ntlm --disable-unix-sockets --disable-socketpair --disable-dateparse --disable-progress-meter --enable-websockets --without-brotli --without-libssh2 --without-libssh --without-wolfssh --without-librtmp
        make
        cd ..
      mv ./curl/src/curl ../bin
      mv ./curl/lib/.libs/libcurl.a ../lib
      mv ./curl/include/* ../include
    cd ..; rm -rf curl

  cd ..

  if command -v apt > /dev/null; then

    echo "Checking for necessary -dev packages..."
    sudo apt install \
      libarchive-dev \
      libxml2-dev \
      liblzma-dev \
      libacl1-dev \
      libgl1-mesa-dev \
      libxcb*-dev \
      libfontconfig1-dev \
      libxkbcommon-x11-dev \
      libpsl-dev \
      libnghttp2-dev \
      libidn2-dev

    echo "Checking for MinGW for cross-compilation..."
    sudo apt install \
      mingw-w64 \
      mingw-w64-tools \
      gcc-mingw-w64-x86-64 \
      g++-mingw-w64-x86-64

    echo "Switching to POSIX threads..."
    sudo update-alternatives --set x86_64-w64-mingw32-gcc /usr/bin/x86_64-w64-mingw32-gcc-posix
    sudo update-alternatives --set x86_64-w64-mingw32-g++ /usr/bin/x86_64-w64-mingw32-g++-posix

  fi
