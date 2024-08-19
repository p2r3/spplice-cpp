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
wget https://github.com/libarchive/libarchive/releases/download/v3.7.4/libarchive-v3.7.4-amd64.zip
unzip libarchive-v3.7.4-amd64.zip
mv ./libarchive/bin/*.exe ../bin
mv ./libarchive/bin/*.dll ../lib
mv ./libarchive/lib/* ../lib
mv ./libarchive/include/* ../include
cd ..; rm -rf archive

mkdir rapidjson; cd rapidjson
wget https://github.com/Tencent/rapidjson/archive/refs/tags/v1.1.0.tar.gz
tar -xf v1.1.0.tar.gz
mv ./rapidjson-1.1.0/include/* ../include
cd ..; rm -rf rapidjson
