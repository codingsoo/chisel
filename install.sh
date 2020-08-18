sudo apt update
sudo apt install -y software-properties-common wget gnupg vim
wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key | apt-key add -
apt-add-repository "deb http://apt.llvm.org/buster/ llvm-toolchain-buster-8 main" && sudo apt update
sudo apt install -y clang-8 libclang-8-dev llvm-8-dev cmake wget
sudo apt install -y libspdlog-dev nlohmann-json-dev libmlpack-dev
sudo apt install -y libpcre3 libpcre3-dev libboost-all-dev
sudo ln -s /usr/bin/clang-8 /usr/bin/clang && ln -s /usr/bin/llvm-config-8 /usr/bin/llvm-config
rm -rf build
mkdir build && cd build && CXX=clang cmake .. && make
