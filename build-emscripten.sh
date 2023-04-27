./autogen.sh
./configure --prefix=/usr --without-async --without-mbrola --without-sonic --with-extdict-ru --with-extdict-cmn --with-extdict-yue
make clean
make

emmake make clean
emconfigure ./configure --prefix=/usr --without-async --without-mbrola --without-sonic --with-extdict-ru --with-extdict-cmn --with-extdict-yue
emmake make src/libespeak-ng.la

cd emscripten
emmake make clean
emmake make