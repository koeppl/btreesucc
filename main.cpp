#ifndef BTREE_SIZE
#define BTREE_EXPAND 1
#define BTREE_SIZE 100000
#define BTREE_Q 30
#define BTREE_T 21
#define BTREE_TP 31
#define BTREE_B 31
#define BTREE_SEED 1
#endif

#if BTREE_EXPAND == 1
#include "btree_expand.hpp"
static constexpr const char*const flavorstr = "expand";
#else
#include "btree_noexpand.hpp"
static constexpr const char*const flavorstr = "noexpand";
#endif

static constexpr int SIZE = BTREE_SIZE;
static constexpr uint32_t q =  BTREE_Q;  // q = 10, 20, 30
static constexpr uint32_t t =  BTREE_T;  // t = 21, 31, 41
static constexpr uint32_t t_ = BTREE_TP; // t_ = 31, 41, 51
static constexpr uint32_t b =  BTREE_B;  // b = 40, 60, 80

#include <iostream>
#include <chrono>
#include <stdio.h>
#include <fstream>

using namespace std;

// パラメータの入力
// t, t_ については内部ノードの実装の都合上、特定の子ノードの数を超えたときに分割が生じるようにするために
// 特定の子ノードの数より1個多く子ノードを確保できるようにする必要がある。そのため20に設定したい時は21, 30に設定したい時は31とする必要がある。
// 1個多く子ノードを保持できるようにしていることで、木構造更新後の静的な状態で子ノードの数が特定の値より1個多く存在することは無い。

// for i in `seq 10000`; do ./total; done
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
    data = new int32_t[SIZE];
    for (int32_t i = 0; i < SIZE; i++)
    {
        data[i] = i;
    }
    shuffle(data, SIZE);

    lb::lb_btree<int32_t, q, b, t, t_> lbb;


    // キーの挿入と計測
    auto start = chrono::high_resolution_clock::now();
    for (int i = 0; i < SIZE; i++)
    {
        lbb.insert(data[i]);
    }
    auto end = chrono::high_resolution_clock::now();
    const int64_t inserttime = chrono::duration_cast<chrono::nanoseconds>(end - start).count();


    // 以下で全キー挿入後に取得したい結果を取得
    auto mem = lbb.byte_size();
    auto height = lbb.height();
    auto internals = lbb.internals();
    auto leaves = lbb.leaves();
    auto mem_a_internal1 = lbb.byte_internal1();
    auto mem_a_internal2 = lbb.byte_internal2();
    auto mem_a_leaf = lbb.byte_leaf();

    // キーの検索と計測
    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < SIZE; i++)
    {
        auto found_value = lbb.find(data[i]);
        if (found_value != data[i])
        {
            std::cerr << "find-error: searching for key " << data[i] << " but we found " << found_value << std::endl;
        }
    }
    end = chrono::high_resolution_clock::now();
    const int64_t findtime = chrono::duration_cast<chrono::nanoseconds>(end - start).count();

    // キーの削除と計測
    start = chrono::high_resolution_clock::now();
    for (int i = 0; i < SIZE; i++)
    {
        lbb.remove(data[i]);
    }
    end = chrono::high_resolution_clock::now();
    const int64_t removetime = chrono::duration_cast<chrono::nanoseconds>(end - start).count();





    // 以下のファイル出力で実行時間を取得
    // ofstream writing_file;
    // string filename = "../output/additional_high_significant/total_SIZE" + to_string(SIZE) + "_q" + to_string(q) + "_t" + to_string(t-1) + "_t_" + to_string(t_-1) + "_b" + to_string(b) + ".txt";
    // writing_file.open(filename, ios::app);
    // string writing_text = to_string(insert) + "," + to_string(remove);
    // writing_file << writing_text << endl;
    // writing_file.close();




    // 以下の標準出力でメモリ使用量を取得
    std::cout << "RESULT "
              << " size=" << SIZE 
							<< " t=" << t - 1 
							<< " tp=" << t_ - 1 
							<< " b=" << b 
							<< " q=" << q 
							<< " flavor=" << flavorstr 
							<< " internals=" << internals
							<< " leaves=" << leaves
							<< " insert_nano=" << inserttime
							<< " find_nano=" << findtime
							<< " remove_nano=" << removetime
							<< " mem_byte=" << mem
							<< " height=" << height
							<< " mem_internal1=" << mem_a_internal1
							<< " mem_internal2=" << mem_a_internal2
							<< " mem_leaf=" << mem_a_leaf
							<< " random_seed=" << BTREE_SEED
							<< std::endl;

    // std::cout << "メモリ使用量：　" << mem / 1024 << " KB" << std::endl;
    // std::cout << "内部ノードの数：　" << internals << " 個" << std::endl;
    // std::cout << "葉ノードの数：　" << leaves << " 個" << std::endl;
    // std::cout << "子ノード（内部）の数の平均：　" << avc << " 個" << std::endl;
    // std::cout << "子ノード（葉）の数の平均：　" << avl << " 個" << std::endl;
    // std::cout << "内部ノード1のメモリサイズ：　" << mem_a_internal1 << "B" << std::endl;
    // std::cout << "内部ノード2のメモリサイズ：　" << mem_a_internal2 << "B" << std::endl;
    // std::cout << "葉ノードのメモリサイズ：　" << mem_a_leaf << "B" << std::endl;
    // std::cout << "木の高さ：　" << height << std::endl;

    delete[] data;
    // }

    return 0;
}
