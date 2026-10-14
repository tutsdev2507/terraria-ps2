/* System.Random do .NET (mscorlib / Mono). Ver src/dotnet_rand.h. */
#include "dotnet_rand.h"

#define MBIG 2147483647
#define MSEED 161803398

/* subtracao com wrap de 32 bits (dois-complemento), como o int do C#. */
static int wsub(int a, int b) { return (int)((unsigned)a - (unsigned)b); }

void dnr_seed(DotNetRandom *r, int seed) {
    /* int subtraction = (Seed == Int32.MinValue) ? Int32.MaxValue : Math.Abs(Seed); */
    int subtraction = (seed == (int)0x80000000) ? MBIG : (seed < 0 ? -seed : seed);
    int mj = MSEED - subtraction;      /* pode ser negativo (bug preservado p/ compat) */
    int mk = 1;
    int i;
    r->seedArray[55] = mj;
    for (i = 1; i < 55; i++) {
        int ii = (21 * i) % 55;
        r->seedArray[ii] = mk;
        mk = wsub(mj, mk);
        if (mk < 0) mk += MBIG;
        mj = r->seedArray[ii];
    }
    for (int k = 1; k < 5; k++) {
        for (i = 1; i < 56; i++) {
            r->seedArray[i] = wsub(r->seedArray[i], r->seedArray[1 + (i + 30) % 55]);
            if (r->seedArray[i] < 0) r->seedArray[i] += MBIG;
        }
    }
    r->inext = 0;
    r->inextp = 21;
}

static int internal_sample(DotNetRandom *r) {
    int locINext = r->inext;
    int locINextp = r->inextp;
    int retVal;
    if (++locINext >= 56) locINext = 1;
    if (++locINextp >= 56) locINextp = 1;
    retVal = wsub(r->seedArray[locINext], r->seedArray[locINextp]);
    if (retVal == MBIG) retVal--;
    if (retVal < 0) retVal += MBIG;
    r->seedArray[locINext] = retVal;
    r->inext = locINext;
    r->inextp = locINextp;
    return retVal;
}

static double sample(DotNetRandom *r) {
    return (double)internal_sample(r) * (1.0 / (double)MBIG);
}

int dnr_next(DotNetRandom *r) { return internal_sample(r); }

int dnr_next_max(DotNetRandom *r, int maxValue) {
    return (int)(sample(r) * (double)maxValue);
}

double dnr_next_double(DotNetRandom *r) { return sample(r); }

/* GetSampleForLargeRange: usado apenas quando o intervalo excede Int32.MaxValue. */
static double get_sample_for_large_range(DotNetRandom *r) {
    int result = internal_sample(r);
    int negative = (internal_sample(r) % 2 == 0) ? 1 : 0;
    double d;
    if (negative) result = -result;
    d = (double)result;
    d += (double)(MBIG - 1);
    d /= (double)(2.0 * (double)(unsigned)MBIG - 1.0);
    return d;
}

int dnr_next_range(DotNetRandom *r, int minValue, int maxValue) {
    long range = (long)maxValue - (long)minValue;
    if (range <= (long)MBIG)
        return (int)(sample(r) * (double)range) + minValue;
    return (int)((long)(get_sample_for_large_range(r) * (double)range) + (long)minValue);
}
