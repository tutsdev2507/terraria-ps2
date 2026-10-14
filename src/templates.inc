/* Gerado por tools/extract_templates.py de data/tutorial.tw (mundo de referencia do APK Terraria mobile).
   CELULAS COPIADAS LITERALMENTE: dx,dy relativos a origem (base da arvore = dy 0, topo do bau = dy 0),
   fx,fy = offsets de sprite (frames important). NAO editar mao. */
typedef struct { signed char dx, dy; unsigned short fx, fy; } WTplCell;
/* arvore 0: 11 celulas, bbox 3x6 */
static const WTplCell wg_tree0[] = {{0,-4,44,242}, {0,-2,44,154}, {0,-1,44,154}, {0,0,44,154}, {1,-5,22,198}, {1,-4,88,44}, {1,-3,66,66}, {1,-2,0,0}, {1,-1,0,22}, {1,0,66,154}, {2,-3,66,220}};
/* arvore 1: 17 celulas, bbox 3x9 */
static const WTplCell wg_tree1[] = {{0,-7,44,220}, {0,-5,44,220}, {0,-4,44,220}, {0,-3,44,220}, {0,-2,44,220}, {0,-1,44,220}, {1,-8,0,220}, {1,-7,88,0}, {1,-6,0,0}, {1,-5,22,22}, {1,-4,22,110}, {1,-3,0,44}, {1,-2,22,88}, {1,-1,110,88}, {1,0,0,154}, {2,-1,88,110}, {2,0,22,154}};
/* arvore 2: 34 celulas, bbox 3x16 */
static const WTplCell wg_tree2[] = {{0,-12,66,22}, {0,-10,44,220}, {0,-9,44,220}, {0,-8,44,220}, {0,-7,44,220}, {0,-6,44,220}, {0,-5,44,220}, {0,-4,44,220}, {0,-3,44,220}, {0,-2,44,220}, {0,-1,44,220}, {0,0,44,176}, {1,-15,22,220}, {1,-14,22,22}, {1,-13,66,110}, {1,-12,88,44}, {1,-11,0,110}, {1,-10,0,0}, {1,-9,44,88}, {1,-8,0,44}, {1,-7,66,110}, {1,-6,0,44}, {1,-5,22,22}, {1,-4,22,22}, {1,-3,22,110}, {1,-2,0,0}, {1,-1,88,22}, {1,0,66,154}, {2,-13,66,242}, {2,-11,66,198}, {2,-10,66,198}, {2,-9,66,198}, {2,-8,66,198}, {2,-7,66,198}};
static const WTplCell *const wg_trees[] = {wg_tree0, wg_tree1, wg_tree2};
static const unsigned wg_tree_count = 3;
static const unsigned wg_tree_sizes[] = {11, 17, 34};
static const unsigned short wg_plant_frames[][2] = {{0,0}, {144,0}, {72,0}, {36,0}, {54,0}, {90,0}, {18,0}, {108,0}, {126,0}};
static const unsigned wg_plant_count = 9;
/* bau 2x2 (topo-esquerda = origem) */
static const WTplCell wg_chest[] = {{0,0,0,0}, {0,1,0,18}, {1,0,18,0}, {1,1,18,18}};
