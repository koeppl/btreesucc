#!/usr/bin/env zsh
set -e
set -x

# for q in 0 2 4 8 16 24 32 48 64; do
# for t in 4
# for tp in 8
# for b in 8;

for seed in 1 2; do
	for q in 0 2 $(seq 4 4 128); do
		for t in $(seq 8 4 128); do
			((tplower=t*2))
			for tp in $(seq $tplower 4 128); do
				((blower=tp*2))
				for b in $(seq $blower 4 1024); do
					for size in $(seq 65536 65536 16777216); do
						for flavor in 1 0; do
							g++ ./main.cpp -Ofast -o a.out \
							-DBTREE_EXPAND=$flavor -DBTREE_SIZE="$size" -DBTREE_Q="$q" -DBTREE_T="$t" -DBTREE_TP="$tp" -DBTREE_B="$b" -DBTREE_SEED="$seed"
							./a.out
						done
					done
				done
			done
		done
	done
done
