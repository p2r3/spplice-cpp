rm -rf win
mkdir win
cd win

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

mkdir curl; cd curl
wget https://curl.se/windows/dl-8.10.1_2/curl-8.10.1_2-win64-mingw.zip
unzip curl-8.10.1_2-win64-mingw.zip
mv curl-8.10.1_2-win64-mingw curl
mv ./curl/bin/*.exe ../bin
mv ./curl/bin/*.dll ../lib
mv ./curl/lib/* ../lib
mv ./curl/include/* ../include
cd ..; rm -rf curl
