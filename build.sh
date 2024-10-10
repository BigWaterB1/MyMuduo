#!/bin/bash

set -e

# 如果没有build目录，则创建
if [ ! -d `pwd`/build ]; then
    mkdir `pwd`/build
fi

rm -rf `pwd`/build/*

cd `pwd`/build/ &&
    cmake .. &&
    make

# 回到根目录
cd ..

# 把头文件拷贝到 /usr/include/myMuduo 把so库拷贝到 /usr/lib
if [ ! -d /usr/include/myMuduo ]; then
    mkdir /usr/include/myMuduo
fi

for header in `ls myMuduo/*.h`
do
    cp $header /usr/include/myMuduo
done

cp `pwd`/lib/libmyMuduo.so /usr/lib

# 刷新动态链接库缓存
ldconfig