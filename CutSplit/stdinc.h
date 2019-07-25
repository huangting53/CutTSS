#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include <stdlib.h>
#include <strings.h>
#include <math.h>

typedef char bit;
// const int false = 0;
// const int true = 1;

const int BIGINT = 0x7fffffff;
const int EOS = '\0';

inline int max(int x, int y) { return x > y ? x : y; }
inline unsigned max(unsigned x, unsigned y) { return x > y ? x : y; }
inline double max(double x, double y) { return x > y ? x : y; }
inline int min(int x, int y) { return x < y ? x : y; }
inline unsigned min(unsigned x, unsigned y) { return x < y ? x : y; }
inline double min(double x, double y) { return x < y ? x : y; }
inline int abs(int x) { return x < 0 ? -x : x; }

inline void warning(char* p) { fprintf(stderr,"Warning:%s \n",p); }
inline void fatal(char* string) {fprintf(stderr,"Fatal:%s\n",string); exit(1); }

//double pow(double,double);
//double log(double);

// long random();
double exp(double),log(double);

// Return a random number in [0,1] 
inline double randfrac() { return ((double) random())/BIGINT; }

// Return a random integer in the range [lo,hi].
// Not very good if range is larger than 10**7.
inline int randint(int lo, int hi) { return lo + (random() % (hi + 1 - lo)); }

// Return a random number from an exponential distribution with mean mu 
inline double randexp(double mu) { return -mu*log(randfrac()); }

// Return a random number from a geometric distribution with mean 1/p
inline int randgeo(double p) { return int(.999999 + log(randfrac())/log(1-p)); }

// Return a random number from a Pareto distribution with mean mu and shape s
inline double randpar(double mu, double s) {
	return mu*(1-1/s)/exp((1/s)*log(randfrac()));
}
