# Simulador de Memoria Cache

Simulador de memoria cache associativa por conjunto com arquitetura configuravel, desenvolvido para a disciplina de Arquitetura de Computadores da Universidade de Caxias do Sul (UCS).

## Compilacao

```bash
gcc -O2 -o simula_cache simula_cache.c -lm
```

## Uso

```bash
./simula_cache <pol_escrita> <tam_linha> <num_linhas> <assoc> <hit_time> <pol_subst> <tempo_mp> <arquivo>
```

**Parametros:**

| Parametro | Descricao |
|-----------|-----------|
| `pol_escrita` | 0 = write-through, 1 = write-back |
| `tam_linha` | Tamanho da linha em bytes (potencia de 2) |
| `num_linhas` | Numero de linhas da cache (potencia de 2) |
| `assoc` | Associatividade por conjunto (potencia de 2) |
| `hit_time` | Tempo de acerto em nanossegundos |
| `pol_subst` | LRU ou ALEATORIA |
| `tempo_mp` | Tempo de leitura/escrita da memoria principal em nanossegundos |
| `arquivo` | Arquivo de enderecos |

**Exemplo:**

```bash
./simula_cache 0 128 64 4 4 LRU 60 official.txt
```

## Estrutura do Projeto

```
simula_cache.c       # Codigo-fonte do simulador
simula_cache         # Executavel compilado (arm64)
teste.txt            # Arquivo de teste (100 enderecos)
official.txt         # Arquivo oficial (51.200 enderecos)
gerar_graficos.py    # Script para gerar graficos (matplotlib)
relatorio/
  main.tex           # Relatorio em LaTeX
  main.pdf           # Relatorio compilado
  figuras/           # Graficos gerados
```

## Relatorio

O relatorio completo com analise dos experimentos esta disponivel em `relatorio/main.pdf`.

## Autor

Pedro Henrique Franca - Universidade de Caxias do Sul (UCS), 2026.
