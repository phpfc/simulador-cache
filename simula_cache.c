#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

/* ===== Tipos ===== */

typedef enum { WRITE_THROUGH, WRITE_BACK } PoliticaEscrita;
typedef enum { SUBST_LRU, SUBST_ALEATORIA } PoliticaSubstituicao;

typedef struct {
    int valido;
    unsigned int rotulo;
    int dirty;
    int lru;
} LinhaCache;

typedef struct {
    PoliticaEscrita politica_escrita;
    int tamanho_linha;
    int num_linhas;
    int associatividade;
    int hit_time;
    PoliticaSubstituicao politica_substituicao;
    int tempo_mp;
    const char *arquivo_enderecos;
} Config;

typedef struct {
    LinhaCache *linhas;
    int num_linhas;
    int num_conjuntos;
    int associatividade;
    int bits_offset;
    int bits_indice;
} Cache;

typedef struct {
    long total_leituras;
    long total_escritas;
    long hits_leitura;
    long hits_escrita;
    long mp_leituras;
    long mp_escritas;
} Estatisticas;

/* ===== Protótipos ===== */

Config       parse_args(int argc, char *argv[]);
Cache       *cria_cache(const Config *cfg);
void         libera_cache(Cache *cache);
unsigned int extrai_indice(const Cache *cache, unsigned int endereco);
unsigned int extrai_rotulo(const Cache *cache, unsigned int endereco);
int          procura_linha(Cache *cache, unsigned int indice, unsigned int rotulo);
int          escolhe_vitima(Cache *cache, unsigned int indice, const Config *cfg);
void         atualiza_lru(Cache *cache, unsigned int indice, int linha_acessada);
void         acessa(Cache *cache, const Config *cfg, unsigned int endereco, char operacao, Estatisticas *st);
void         finaliza_simulacao(Cache *cache, const Config *cfg, Estatisticas *st);
void         escreve_resultados(const Config *cfg, const Estatisticas *st);

/* ===== Funções auxiliares ===== */

static int log2_inteiro(int n) {
    int r = 0;
    while (n > 1) { n >>= 1; r++; }
    return r;
}

/* ===== parse_args ===== */

Config parse_args(int argc, char *argv[]) {
    if (argc != 9) {
        fprintf(stderr, "Uso: %s <pol_escrita> <tam_linha> <num_linhas> <assoc> <hit_time> <pol_subst> <tempo_mp> <arquivo>\n", argv[0]);
        fprintf(stderr, "  pol_escrita: 0 = write-through, 1 = write-back\n");
        fprintf(stderr, "  tam_linha:   tamanho da linha em bytes (potencia de 2)\n");
        fprintf(stderr, "  num_linhas:  numero de linhas (potencia de 2)\n");
        fprintf(stderr, "  assoc:       associatividade (potencia de 2)\n");
        fprintf(stderr, "  hit_time:    tempo de acerto em ns\n");
        fprintf(stderr, "  pol_subst:   LRU ou ALEATORIA\n");
        fprintf(stderr, "  tempo_mp:    tempo de leitura/escrita da MP em ns\n");
        fprintf(stderr, "  arquivo:     arquivo de enderecos\n");
        exit(1);
    }

    Config cfg;
    cfg.politica_escrita = atoi(argv[1]) == 0 ? WRITE_THROUGH : WRITE_BACK;
    cfg.tamanho_linha = atoi(argv[2]);
    cfg.num_linhas = atoi(argv[3]);
    cfg.associatividade = atoi(argv[4]);
    cfg.hit_time = atoi(argv[5]);

    if (strcmp(argv[6], "LRU") == 0)
        cfg.politica_substituicao = SUBST_LRU;
    else
        cfg.politica_substituicao = SUBST_ALEATORIA;

    cfg.tempo_mp = atoi(argv[7]);
    cfg.arquivo_enderecos = argv[8];

    return cfg;
}

/* ===== cria_cache / libera_cache ===== */

Cache *cria_cache(const Config *cfg) {
    Cache *cache = malloc(sizeof(Cache));
    cache->num_linhas = cfg->num_linhas;
    cache->associatividade = cfg->associatividade;
    cache->num_conjuntos = cfg->num_linhas / cfg->associatividade;
    cache->bits_offset = log2_inteiro(cfg->tamanho_linha);
    cache->bits_indice = log2_inteiro(cache->num_conjuntos);
    cache->linhas = calloc(cfg->num_linhas, sizeof(LinhaCache));
    return cache;
}

void libera_cache(Cache *cache) {
    free(cache->linhas);
    free(cache);
}

/* ===== Decomposição de endereço ===== */

unsigned int extrai_indice(const Cache *cache, unsigned int endereco) {
    return (endereco >> cache->bits_offset) & (cache->num_conjuntos - 1);
}

unsigned int extrai_rotulo(const Cache *cache, unsigned int endereco) {
    return endereco >> (cache->bits_offset + cache->bits_indice);
}

/* ===== Busca e substituição ===== */

int procura_linha(Cache *cache, unsigned int indice, unsigned int rotulo) {
    int base = indice * cache->associatividade;
    for (int i = 0; i < cache->associatividade; i++) {
        LinhaCache *l = &cache->linhas[base + i];
        if (l->valido && l->rotulo == rotulo)
            return base + i;
    }
    return -1;
}

int escolhe_vitima(Cache *cache, unsigned int indice, const Config *cfg) {
    int base = indice * cache->associatividade;

    /* Priorizar slot inválido */
    for (int i = 0; i < cache->associatividade; i++) {
        if (!cache->linhas[base + i].valido)
            return base + i;
    }

    if (cfg->politica_substituicao == SUBST_LRU) {
        int vitima = base;
        int max_lru = cache->linhas[base].lru;
        for (int i = 1; i < cache->associatividade; i++) {
            if (cache->linhas[base + i].lru > max_lru) {
                max_lru = cache->linhas[base + i].lru;
                vitima = base + i;
            }
        }
        return vitima;
    } else {
        return base + (rand() % cache->associatividade);
    }
}

void atualiza_lru(Cache *cache, unsigned int indice, int linha_acessada) {
    int base = indice * cache->associatividade;
    for (int i = 0; i < cache->associatividade; i++) {
        if (base + i == linha_acessada)
            cache->linhas[base + i].lru = 0;
        else if (cache->linhas[base + i].valido)
            cache->linhas[base + i].lru++;
    }
}

/* ===== Núcleo da simulação ===== */

void acessa(Cache *cache, const Config *cfg, unsigned int endereco, char operacao, Estatisticas *st) {
    unsigned int indice = extrai_indice(cache, endereco);
    unsigned int rotulo = extrai_rotulo(cache, endereco);
    int pos;

    if (operacao == 'R') {
        st->total_leituras++;
        pos = procura_linha(cache, indice, rotulo);
        if (pos >= 0) {
            /* Hit de leitura */
            st->hits_leitura++;
            atualiza_lru(cache, indice, pos);
        } else {
            /* Miss de leitura */
            st->mp_leituras++;
            int vitima = escolhe_vitima(cache, indice, cfg);
            if (cfg->politica_escrita == WRITE_BACK &&
                cache->linhas[vitima].valido && cache->linhas[vitima].dirty) {
                st->mp_escritas++;
            }
            cache->linhas[vitima].valido = 1;
            cache->linhas[vitima].rotulo = rotulo;
            cache->linhas[vitima].dirty = 0;
            atualiza_lru(cache, indice, vitima);
        }
    } else { /* W */
        st->total_escritas++;

        if (cfg->politica_escrita == WRITE_THROUGH) {
            /* Write-through: sempre escreve na MP */
            st->mp_escritas++;
            pos = procura_linha(cache, indice, rotulo);
            if (pos >= 0) {
                st->hits_escrita++;
                atualiza_lru(cache, indice, pos);
            } else {
                /* Write-non-allocate: carrega o bloco na cache */
                st->mp_leituras++;
                int vitima = escolhe_vitima(cache, indice, cfg);
                cache->linhas[vitima].valido = 1;
                cache->linhas[vitima].rotulo = rotulo;
                cache->linhas[vitima].dirty = 0;
                atualiza_lru(cache, indice, vitima);
            }
        } else {
            /* Write-back: write-allocate */
            pos = procura_linha(cache, indice, rotulo);
            if (pos >= 0) {
                st->hits_escrita++;
                cache->linhas[pos].dirty = 1;
                atualiza_lru(cache, indice, pos);
            } else {
                st->mp_leituras++;
                int vitima = escolhe_vitima(cache, indice, cfg);
                if (cache->linhas[vitima].valido && cache->linhas[vitima].dirty) {
                    st->mp_escritas++;
                }
                cache->linhas[vitima].valido = 1;
                cache->linhas[vitima].rotulo = rotulo;
                cache->linhas[vitima].dirty = 1;
                atualiza_lru(cache, indice, vitima);
            }
        }
    }
}

/* ===== Finalização ===== */

void finaliza_simulacao(Cache *cache, const Config *cfg, Estatisticas *st) {
    if (cfg->politica_escrita == WRITE_BACK) {
        for (int i = 0; i < cache->num_linhas; i++) {
            if (cache->linhas[i].valido && cache->linhas[i].dirty)
                st->mp_escritas++;
        }
    }
}

/* ===== Saída ===== */

void escreve_resultados(const Config *cfg, const Estatisticas *st) {
    long total = st->total_leituras + st->total_escritas;
    long hits_total = st->hits_leitura + st->hits_escrita;
    double taxa_leitura = st->total_leituras > 0 ? (double)st->hits_leitura / st->total_leituras : 0.0;
    double taxa_escrita = st->total_escritas > 0 ? (double)st->hits_escrita / st->total_escritas : 0.0;
    double taxa_global = total > 0 ? (double)hits_total / total : 0.0;
    double amat = cfg->hit_time + (1.0 - taxa_global) * cfg->tempo_mp;

    printf("=== Parametros de Entrada ===\n");
    printf("Politica de escrita: %s\n", cfg->politica_escrita == WRITE_THROUGH ? "write-through" : "write-back");
    printf("Tamanho da linha: %d bytes\n", cfg->tamanho_linha);
    printf("Numero de linhas: %d\n", cfg->num_linhas);
    printf("Associatividade: %d\n", cfg->associatividade);
    printf("Tempo de hit: %d ns\n", cfg->hit_time);
    printf("Politica de substituicao: %s\n", cfg->politica_substituicao == SUBST_LRU ? "LRU" : "Aleatoria");
    printf("Tempo da MP: %d ns\n", cfg->tempo_mp);
    printf("Arquivo de enderecos: %s\n", cfg->arquivo_enderecos);

    printf("\n=== Enderecos ===\n");
    printf("Total de leituras: %ld\n", st->total_leituras);
    printf("Total de escritas: %ld\n", st->total_escritas);
    printf("Total de enderecos: %ld\n", total);

    printf("\n=== Memoria Principal ===\n");
    printf("Leituras na MP: %ld\n", st->mp_leituras);
    printf("Escritas na MP: %ld\n", st->mp_escritas);

    printf("\n=== Taxa de Acerto ===\n");
    printf("Leitura: %.4f (%ld/%ld)\n", taxa_leitura, st->hits_leitura, st->total_leituras);
    printf("Escrita: %.4f (%ld/%ld)\n", taxa_escrita, st->hits_escrita, st->total_escritas);
    printf("Global:  %.4f (%ld/%ld)\n", taxa_global, hits_total, total);

    printf("\n=== Tempo Medio de Acesso ===\n");
    printf("AMAT: %.4f ns\n", amat);
}

/* ===== main ===== */

int main(int argc, char *argv[]) {
    srand(42);

    Config cfg = parse_args(argc, argv);
    Cache *cache = cria_cache(&cfg);
    Estatisticas st = {0};

    FILE *fp = fopen(cfg.arquivo_enderecos, "r");
    if (!fp) {
        fprintf(stderr, "Erro ao abrir arquivo: %s\n", cfg.arquivo_enderecos);
        libera_cache(cache);
        return 1;
    }

    unsigned int endereco;
    char operacao;
    while (fscanf(fp, "%x %c", &endereco, &operacao) == 2) {
        acessa(cache, &cfg, endereco, operacao, &st);
    }
    fclose(fp);

    finaliza_simulacao(cache, &cfg, &st);
    escreve_resultados(&cfg, &st);
    libera_cache(cache);

    return 0;
}
