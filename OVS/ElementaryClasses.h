#ifndef  ELEM_H
#define  ELEM_H
#include <vector>
#include <queue>
#include <list>
#include <set>
#include <map>
#include <unordered_map>
#include <iostream>
#include <algorithm>
#include <random>
#include <numeric>
#include <memory>
#include <chrono> 
#include <array>
#include "MapExtensions.h"

#define MAXDIMENSIONS 5
#define MAXCUTS 16
#define MAXNODES 5000000
#define MAXRULES 1000000
#define MAXBUCKETS 40
#define RULE_SIZE 16
#define NODE_SIZE 16
#define LEAF_NODE_SIZE 4
#define TREE_NODE_SIZE 8
#define Null -1
#define PTR_SIZE 4
#define HEADER_SIZE 4


#define FieldSA 0
#define FieldDA 1
#define FieldSP 2
#define FieldDP 3
#define FieldProto 4

#define LowDim 0
#define HighDim 1

#define POINT_SIZE_BITS 32

typedef uint32_t Point;
typedef std::vector<Point> Packet;
typedef uint32_t Memory;



struct Rule
{
	//Rule(){};
	Rule(int dim = 5) : dim(dim), range(dim, { { 0, 0 } }), prefix_length(dim, 0){ markedDelete = 0; }
 
	int dim;
	int	priority;

	int id;
	int tag;
	bool markedDelete = 0;

	std::vector<unsigned> prefix_length;

	std::vector<std::array<Point,2>> range;

	bool inline MatchesPacket(const Packet& p) const {
		for (int i = 0; i < dim; i++) {
			if ((p[i] < range[i][LowDim]) || p[i] > range[i][HighDim]) return false;
		}
		return true;
	}

	void Print() const {
		for (int i = 0; i < dim; i++) {
			printf("%u:%u ", range[i][LowDim], range[i][HighDim]);
		}
		printf("\n");
	}
};

class PacketClassifier {
public:
	virtual void ConstructClassifier(const std::vector<Rule>& rules) = 0;
	virtual int ClassifyAPacket(const Packet& packet) = 0;
	virtual void DeleteRule(const Rule& rule) = 0;
	virtual void InsertRule(const Rule& rule) = 0;
	virtual Memory MemSizeBytes() const = 0;
	virtual int MemoryAccess() const = 0;
	virtual size_t NumTables() const = 0;
	virtual size_t RulesInTable(size_t tableIndex) const = 0;

	int TablesQueried() const {	return queryCount; }
	int NumPacketsQueriedNTables(int n) const { return GetOrElse<int, int>(packetHistogram, n, 0); };

protected:
	void QueryUpdate(int query) {
		packetHistogram[query]++;
		queryCount += query;
	}

private:
	int queryCount = 0;
	std::unordered_map<int, int> packetHistogram;
};

inline void SortRules(std::vector<Rule>& rules) {
	sort(rules.begin(), rules.end(), [](const Rule& rx, const Rule& ry) { return rx.priority >= ry.priority; });
}

inline void SortRules(std::vector<Rule*>& rules) {
	sort(rules.begin(), rules.end(), [](const Rule* rx, const Rule* ry) { return rx->priority >= ry->priority; });
}
class Interval {
public:
	Interval() {}
	virtual Point GetLowPoint() const = 0;
	virtual Point GetHighPoint() const = 0;
	virtual void Print() const=0;
};

class interval : public Interval {
public:
	interval(unsigned int a, unsigned int b, int id) : a(a), b(b), id(id) {}
	Point GetLowPoint() const { return a; }
	Point GetHighPoint() const { return b; }
	void Print()const {};

	Point a, b;
	bool operator < (const interval& rhs) const {
		if (a != rhs.a) {
			return a < rhs.a;
		} else return b < rhs.b;
	}
	bool operator == (const interval& rhs) const {
		return a == rhs.a && b == rhs.b;
	}
	int id;
	int weight;

};
struct EndPoint {
	EndPoint(double val, bool isRightEnd, int id) : val(val), isRightEnd(isRightEnd), id(id){}
	bool operator < (const EndPoint & rhs) const {
		return val < rhs.val;
	}
	double val;
	bool isRightEnd;
	int id;
};

//class Random {
//public:
//	// random number generator from Stroustrup:
//	// http://www.stroustrup.com/C++11FAQ.html#std-random
//	// static: there is only one initialization (and therefore seed).
//	static int random_int(int low, int high)
//	{
//		//static std::mt19937  generator;
//		using Dist = std::uniform_int_distribution < int >;
//		static Dist uid{};
//		return uid(generator, Dist::param_type{ low, high });
//	}
//
//	// random number generator from Stroustrup:
//	// http://www.stroustrup.com/C++11FAQ.html#std-random
//	// static: there is only one initialization (and therefore seed).
//	static int random_unsigned_int()
//	{
//		//static std::mt19937  generator;
//		using Dist = std::uniform_int_distribution < unsigned int >;
//		static Dist uid{};
//		return uid(generator, Dist::param_type{ 0, 4294967295 });
//	}
//	static double random_real_btw_0_1()
//	{
//		//static std::mt19937  generator;
//		using Dist = std::uniform_real_distribution < double >;
//		static Dist uid{};
//		return uid(generator, Dist::param_type{ 0,1 });
//	}
//
//	template <class T>
//	static std::vector<T> shuffle_vector(std::vector<T> vi) {
//		//static std::mt19937  generator;
//		std::shuffle(std::begin(vi), std::end(vi), generator);
//		return vi;
//	}
//private:
//	static std::mt19937 generator;
//};

#endif
