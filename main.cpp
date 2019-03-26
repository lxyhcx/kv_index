#include <iostream>
#include "kvindex.h"

using namespace std;

int main(int argc, char *argv[])
{
    cout << "Hello World!" << endl;
    string index_file("test.idx");
    kvindex::Index* index;
    kvindex::Index::Open(index_file, 1048576, &index);

    uint64_t offset;
    index->Put("aa", 2, 3);
    kvindex::ErrorCode code = index->Get("aa", 2, &offset);
    cout << code << endl;
    return 0;
}
