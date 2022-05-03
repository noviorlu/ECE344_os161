# cd ~/ece344/os161
# ./configure --werror --ostree=$HOME/ece344/build

# cd ~/ece344/os161
# cd kern/conf
# ./config ASST4

cd ~/ece344/os161/kern
cd compile/ASST4
make depend
make
make install