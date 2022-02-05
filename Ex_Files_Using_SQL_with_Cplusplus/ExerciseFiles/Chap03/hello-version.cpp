//  hello-version.cpp
//  Copyright 2021 BHG [bw.org]
//  as of 2021-03-23 bw

#include <cstdio>
#include <sqlite3.h>

int main() {
    printf("sqlite3 version: %s\n", sqlite3_libversion());
    printf("sqlite3 source ID: %s\n", sqlite3_sourceid());

    return 0;
}
