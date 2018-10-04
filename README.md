# Key-value datastructures

This repository is an expanding set of implementations of key-value stores, often drawn from recent
papers.

This is not production ready code! It's lightly tested, missing features and may have known bugs. It
*might* be useful to look and learn something from.

## What's here

Two stores are included right now:

* A very basic B-tree implementation (see
  [/b-tree](https://github.com/henryr/key-value-datastructures/tree/master/b-tree))
* An implementation of MICA's in-memory key-value store to support [this blog
  post](https://www.the-paper-trail.org/post/mica-paper-notes/) (see
  [/formica](https://github.com/henryr/key-value-datastructures/tree/master/formica))

## How to build

    git clone https://github.com/henryr/key-value-datastructures.git ./key-value-datastructures
    cd key-value-datastructures/

    # Download and build thirdparty libraries: googletest and google benchmark
    mkdir thirdparty && cd thirdparty
    wget https://github.com/google/googletest/archive/release-1.8.1.tar.gz
    tar xvzf ./release-1.8.1.tar.gz
    cd googletest-release-1.8.1
    cmake -DCMAKE_INSTALL_PREFIX=`pwd`/../gtest-1.8.0/
    make && make install

    cd ../ # should be <srcdir>/thirdparty
    wget https://github.com/google/benchmark/archive/v1.4.1.tar.gz
    tar xvzf v1.4.1.tar.gz
    cd benchmark-1.4.1
    cmake -DCMAKE_INSTALL_PREFIX=`pwd`/../google-benchmark/ -DBENCHMARK_ENABLE_GTEST_TESTS=OFF
    make && make install

    cd ../../ # Should be <srcdir>
    cmake .
    make
