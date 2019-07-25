CPP=g++
CFLAGS = -g -w -std=c++11 -fpermissive -O3 $(INCLUDE)

# LIBS = -lghthash
main:main.cpp CutTSS.cpp BitTSS.cpp ./TSS/tss.cpp ./OVS/TupleSpaceSearch.cpp ./OVS/cmap.cpp ./OVS/MapExtensions.cpp  ./CutSplit/hs.cpp ./CutSplit/trie.cpp ./CutSplit/CutSplit.cpp ./PartitionSort/misc.cpp ./PartitionSort/OptimizedMITree.cpp ./PartitionSort/PartitionSort.cpp ./PartitionSort/red_black_tree.cpp ./PartitionSort/SortableRulesetPartitioner.cpp ./PartitionSort/stack.cpp ./PartitionSort/test_red_black_tree.cpp ./PartitionSort/IntervalUtilities.cpp
	${CPP} ${CFLAGS} -o main main.cpp CutTSS.cpp BitTSS.cpp ./TSS/tss.cpp ./OVS/TupleSpaceSearch.cpp ./OVS/cmap.cpp ./OVS/MapExtensions.cpp ./CutSplit/hs.cpp ./CutSplit/trie.cpp ./CutSplit/CutSplit.cpp ./PartitionSort/misc.cpp ./PartitionSort/OptimizedMITree.cpp ./PartitionSort/PartitionSort.cpp ./PartitionSort/red_black_tree.cpp ./PartitionSort/SortableRulesetPartitioner.cpp ./PartitionSort/stack.cpp ./PartitionSort/test_red_black_tree.cpp ./PartitionSort/IntervalUtilities.cpp -lghthash

all:main
clean:
	rm main
