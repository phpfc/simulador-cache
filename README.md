# Simulador de Memória Cache

Simulador de memória cache associativa por conjunto com arquitetura configurável, desenvolvido para a disciplina de Fundamentos de Arquitetura de Computadores da Universidade de Caxias do Sul (UCS).

## Compilação

```bash
gcc -O2 -o simula_cache simula_cache.c -lm
```

## Uso

```bash
./simula_cache <pol_escrita> <tam_linha> <num_linhas> <assoc> <hit_time> <pol_subst> <tempo_mp> <arquivo>
```

**Parâmetros:**

| Parâmetro | Descrição |
|-----------|-----------|
| `pol_escrita` | 0 = write-through, 1 = write-back |
| `tam_linha` | Tamanho da linha em bytes (potência de 2) |
| `num_linhas` | Número de linhas da cache (potência de 2) |
| `assoc` | Associatividade por conjunto (potência de 2) |
| `hit_time` | Tempo de acerto em nanossegundos |
| `pol_subst` | LRU ou ALEATORIA |
| `tempo_mp` | Tempo de leitura/escrita da memória principal em nanossegundos |
| `arquivo` | Arquivo de endereços |

**Exemplo:**

```bash
./simula_cache 0 128 64 4 4 LRU 60 official.txt
```

## Estrutura do Projeto

```
simula_cache.c       # Código-fonte do simulador
simula_cache         # Executável compilado (arm64)
teste.txt            # Arquivo de teste (100 endereços)
official.txt         # Arquivo oficial (51.200 endereços)
relatorio/
  main.pdf           # Relatório compilado
  figuras/           # Gráficos gerados
```

## Relatório

O relatório completo com análise dos experimentos está disponível em `relatorio/main.pdf`.

## Autor

Pedro Henrique França — Universidade de Caxias do Sul (UCS), 2026.
