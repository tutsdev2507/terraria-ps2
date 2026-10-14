/* Teste do RNG do .NET (make -f Makefile.host dotnet_rand_test && ./dotnet_rand_test).
 * Valida contra vetores conhecidos e contra a formula fechada do 1o Next():
 *   new Random(s).Next() = (1121899819*|s| + 1559595546) mod (2^31-1). */
#include <stdio.h>
#include <stdlib.h>
#include "dotnet_rand.h"

#define MBIG 2147483647

static int failures;
#define CHECK(cond,...) do{ if(!(cond)){ printf("  FALHA: " __VA_ARGS__); printf("\n"); failures++; } }while(0)

static long long nrn(long long s){
    if (s < 0) s = -s;
    return (1121899819LL * s + 1559595546LL) % MBIG;
}

int main(void){
    DotNetRandom r;

    /* vetores publicos conhecidos de new Random(0) */
    dnr_seed(&r, 0);
    int a = dnr_next(&r), b = dnr_next(&r);
    printf("Random(0).Next() = %d, %d\n", a, b);
    CHECK(a == 1559595546, "1o Next(seed=0) = %d (esperado 1559595546)", a);
    CHECK(b == 1755192844, "2o Next(seed=0) = %d (esperado 1755192844)", b);

    /* formula fechada do 1o Next() para muitas sementes */
    int bad = 0;
    for (int s = 0; s <= 5000; s++) {
        dnr_seed(&r, s);
        int v = dnr_next(&r);
        if (v != (int)nrn(s)) { if (bad < 5) printf("  seed=%d got=%d exp=%lld\n", s, v, nrn(s)); bad++; }
    }
    for (int s = -1; s >= -5000; s--) {
        dnr_seed(&r, s);
        int v = dnr_next(&r);
        if (v != (int)nrn(s)) { if (bad < 5) printf("  seed=%d got=%d exp=%lld\n", s, v, nrn(s)); bad++; }
    }
    CHECK(bad == 0, "%d sementes com 1o Next() divergente da formula", bad);

    /* int.MinValue nao pode quebrar (Math.Abs tratado) */
    dnr_seed(&r, (int)0x80000000);
    int v = dnr_next(&r);
    CHECK(v == (int)nrn(MBIG), "seed=Int32.MinValue Next()=%d (esperado %lld)", v, nrn(MBIG));

    /* determinismo */
    DotNetRandom r2;
    dnr_seed(&r, 12345); dnr_seed(&r2, 12345);
    int ok = 1;
    for (int i = 0; i < 100000; i++) if (dnr_next(&r) != dnr_next(&r2)) { ok = 0; break; }
    CHECK(ok, "sequencia nao deterministica com mesma semente");

    /* faixas: Next(0,max) em [0,max); Next(min,max) em [min,max) */
    dnr_seed(&r, 42);
    int inrange = 1;
    for (int i = 0; i < 200000; i++) {
        int x = dnr_next_max(&r, 100);
        if (x < 0 || x >= 100) inrange = 0;
        int y = dnr_next_range(&r, -7, 7);
        if (y < -7 || y >= 7) inrange = 0;
    }
    CHECK(inrange, "valores fora do intervalo em Next(max)/Next(min,max)");

    if (failures) { printf("\n%d FALHA(S) no RNG do .NET\n", failures); return 1; }
    printf("\nOK: RNG do .NET identico ao .NET/Mono\n");
    return 0;
}
