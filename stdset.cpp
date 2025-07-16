#ifndef BTREE_SEED
#define BTREE_SEED 1
#endif
#ifndef BTREE_SIZE
#define BTREE_SIZE 100000
#endif

#ifndef MALLOC_COUNT
#define MALLOC_COUNT 0
#endif
#if MALLOC_COUNT == 1
#include "malloc_count.h"
#endif

#include <iostream>
#include <chrono>
#include <stdio.h>
#include <fstream>
#include <set>

using namespace std;

void shuffle(int32_t ary[], int size)
{
    srand(BTREE_SEED);
    for (int32_t i = 0; i < size; i++)
    {
        int32_t j = rand() % size;
        int32_t t = ary[i];
        ary[i] = ary[j];
        ary[j] = t;
    }
}

int main()
{
    // ランダムな順のキー生成
    int32_t *data;
    data = new int32_t[BTREE_SIZE];
    for (int32_t i = 0; i < BTREE_SIZE; i++)
    {
        data[i] = i;
    }
    shuffle(data, BTREE_SIZE);

    std::set<int32_t> lbb;


    // キーの挿入と計測
    auto start = chrono::high_resolution_clock::now();
    for (int i = 0; i < BTREE_SIZE; i++)
    {
        lbb.insert(data[i]);
    }
    auto end = chrono::high_resolution_clock::now();
    const int64_t inserttime = chrono::duration_cast<chrono::nanoseconds>(end - start).count();

    // キーの検索と計測
    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < BTREE_SIZE; i++)
    {
        auto ret_itr = lbb.find(data[i]);
        if (ret_itr == lbb.end())
        {
            std::cerr << "find-error: searching for key " << data[i] << " but not found." << std::endl;
        }
    }
    end = chrono::high_resolution_clock::now();
    const int64_t findtime = chrono::duration_cast<chrono::nanoseconds>(end - start).count();

    // キーの削除と計測
    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < BTREE_SIZE; i++)
    {
        lbb.erase(data[i]);
    }
    end = chrono::high_resolution_clock::now();
    const int64_t removetime = chrono::duration_cast<chrono::nanoseconds>(end - start).count();


    // 以下の標準出力でメモリ使用量を取得
    std::cout << "RESULT "
              << " size=" << BTREE_SIZE 
							<< " flavor=stdset" 
#if MALLOC_COUNT == 1
                            << "_malloc_count ds_peak_bytes=" << malloc_count_peak() - BTREE_SIZE * sizeof(data[1])
#endif
							<< " insert_nano=" << inserttime
   							<< " find_nano=" << findtime
							<< " remove_nano=" << removetime
							<< " random_seed=" << BTREE_SEED
							<< std::endl;

    delete[] data;

    return 0;
}
