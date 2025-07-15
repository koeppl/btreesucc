#!/usr/bin/env bash
#set -e
set -x

REPEATS=100

seed=1

for size in 1310720 2359296 3407872; do
	g++ ./stdset.cpp -o stdset.out \
		-DBTREE_EXPAND=$flavor -DBTREE_SIZE="$size" -DBTREE_Q="$q" -DBTREE_T="$t" -DBTREE_TP="$tp" -DBTREE_B="$b" -DBTREE_SEED="$seed"
			./stdset.out >/dev/null
	[[ "$?" -eq 0 ]] || continue
	g++ -Ofast -DNDEBUG ./stdset.cpp -o stdset.out \
		-DBTREE_EXPAND=$flavor -DBTREE_SIZE="$size" -DBTREE_Q="$q" -DBTREE_T="$t" -DBTREE_TP="$tp" -DBTREE_B="$b" -DBTREE_SEED="$seed"
	for i in $(seq 1 $REPEATS); do
		./stdset.out
	done
done



# for size in $(seq 262144 1048576 16777216); do
for size in 1310720 2359296 3407872; do
	for q in 0 2 4 8 16 24 32 48 64; do
		for t in $(seq 8 16 128); do
			((tplower=t*2))
			for tp in $(seq $tplower 32 256) 512; do
				((blower=tp*2))
				for b in $(seq $blower 64 256) 1024; do
						g++ ./main.cpp -o a.out \
						-DBTREE_EXPAND=0 -DBTREE_SIZE="$size" -DBTREE_Q="$q" -DBTREE_T="$t" -DBTREE_TP="$tp" -DBTREE_B="$b" -DBTREE_SEED="$seed"
						./a.out >/dev/null
						[[ "$?" -eq 0 ]] || continue
						g++ -Ofast -DNDEBUG ./main.cpp -o a.out \
						-DBTREE_EXPAND=0 -DBTREE_SIZE="$size" -DBTREE_Q="$q" -DBTREE_T="$t" -DBTREE_TP="$tp" -DBTREE_B="$b" -DBTREE_SEED="$seed"
						for i in $(seq 1 $REPEATS); do
							./a.out
						done
					done
				done
			done
		done
done
