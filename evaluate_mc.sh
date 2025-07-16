#!/usr/bin/env bash
#set -e
set -x

MALLOC_COUNT_PATH=./malloc_count/
REPEATS=100

seed=1

gcc ${MALLOC_COUNT_PATH}malloc_count.c -c -o malloc_count.o

for size in 1310720 2359296 3407872; do
	for mc in 1 0; do
		g++ ./stdset.cpp -c -o stdset.o -I$MALLOC_COUNT_PATH \
			-DMALLOC_COUNT="$mc" -DBTREE_EXPAND=$flavor -DBTREE_SIZE="$size" -DBTREE_Q="$q" -DBTREE_T="$t" -DBTREE_TP="$tp" -DBTREE_B="$b" -DBTREE_SEED="$seed"
		g++ -o stdset.out ./stdset.o ./malloc_count.o -ldl
		./stdset.out >/dev/null
		[[ "$?" -eq 0 ]] || continue
		g++ ./stdset.cpp -c -o stdset.o -I$MALLOC_COUNT_PATH \
			-DMALLOC_COUNT="$mc" -DBTREE_EXPAND=$flavor -DBTREE_SIZE="$size" -DBTREE_Q="$q" -DBTREE_T="$t" -DBTREE_TP="$tp" -DBTREE_B="$b" -DBTREE_SEED="$seed"
		g++ -o stdset.out ./stdset.o ./malloc_count.o -ldl
		for i in $(seq 1 $REPEATS); do
			./stdset.out
			if [ "$mc" -eq 1 ]; then
				break
			fi
		done
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
					for mc in 1 0; do
						g++ ./main.cpp -c -o a.o -I$MALLOC_COUNT_PATH \
						-DMALLOC_COUNT="$mc" -DBTREE_EXPAND=0 -DBTREE_SIZE="$size" -DBTREE_Q="$q" -DBTREE_T="$t" -DBTREE_TP="$tp" -DBTREE_B="$b" -DBTREE_SEED="$seed"
						g++ -o a.out ./a.o ./malloc_count.o -ldl
						./a.out >/dev/null
						[[ "$?" -eq 0 ]] || continue
						g++ -Ofast -DNDEBUG ./main.cpp -c -o a.o -I$MALLOC_COUNT_PATH \
						-DMALLOC_COUNT="$mc" -DBTREE_EXPAND=0 -DBTREE_SIZE="$size" -DBTREE_Q="$q" -DBTREE_T="$t" -DBTREE_TP="$tp" -DBTREE_B="$b" -DBTREE_SEED="$seed"
						g++ -o a.out ./a.o ./malloc_count.o -ldl
						for i in $(seq 1 $REPEATS); do
							./a.out
							[[ "$mc" -eq 1 ]] || break
						done
					done
				done
			done
		done
	done
done
