/* Reproducao exata do System.Random do .NET Framework/Mono (algoritmo
 * subtrativo de Knuth) usado pelo Terraria mobile 1.2 (2016).
 *
 * Necessario para portar a geracao de mundo 1:1: a sequencia de sorteios
 * (genRand) define todo o mundo. Validado contra vetores conhecidos do .NET:
 * new Random(0).Next() == 1559595546, depois 1755192844.
 *
 * Ver src/dotnet_rand.c e docs/re_port_plan.md. */
#ifndef DOTNET_RAND_H
#define DOTNET_RAND_H
#ifdef __cplusplus
extern "C" {
#endif
typedef struct { int seedArray[56]; int inext, inextp; } DotNetRandom;

/* Equivalente a new Random(seed). */
void dnr_seed(DotNetRandom *r, int seed);
/* Equivalente a r.Next(). */
int  dnr_next(DotNetRandom *r);
/* Equivalente a r.Next(0, maxValue). */
int  dnr_next_max(DotNetRandom *r, int maxValue);
/* Equivalente a r.Next(minValue, maxValue). */
int  dnr_next_range(DotNetRandom *r, int minValue, int maxValue);
/* Equivalente a r.NextDouble(). */
double dnr_next_double(DotNetRandom *r);

#ifdef __cplusplus
}
#endif
#endif
