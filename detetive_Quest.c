#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_NOME 64
#define MAX_PISTA 128
#define HASH_TABLE_SIZE 101

/* ===========================
   ESTRUTURAS
   =========================== */

/* Nó da árvore da mansão (cada sala) */
typedef struct Sala {
    char nome[MAX_NOME];
    char pista[MAX_PISTA]; /* pista associada a essa sala (pode estar vazia) */
    struct Sala *esq;
    struct Sala *dir;
} Sala;

/* Nó da BST que armazena pistas coletadas (ordenação por string) */
typedef struct PistaNode {
    char pista[MAX_PISTA];
    struct PistaNode *esq;
    struct PistaNode *dir;
} PistaNode;

/* Entrada da tabela hash: chave = pista, valor = nome do suspeito */
typedef struct HashEntry {
    char pista[MAX_PISTA];
    char suspeito[MAX_NOME];
    struct HashEntry *prox;
} HashEntry;

/* A tabela hash é um vetor de ponteiros para HashEntry (lista encadeada por bucket) */
typedef struct {
    HashEntry *buckets[HASH_TABLE_SIZE];
} HashTable;

/* ===========================
   FUNCOES DE SALA (MANSÃO)
   =========================== */

/*
 * criarSala() – cria dinamicamente um cômodo com nome e pista (pista pode ser "")
 * retorna: ponteiro para a Sala alocada
 */
Sala* criarSala(const char *nome, const char *pista) {
    Sala *s = (Sala*) malloc(sizeof(Sala));
    if (!s) {
        fprintf(stderr, "Erro: falha na alocacao de sala\n");
        exit(1);
    }
    strncpy(s->nome, nome, MAX_NOME - 1);
    s->nome[MAX_NOME - 1] = '\0';
    if (pista && pista[0] != '\0')
        strncpy(s->pista, pista, MAX_PISTA - 1);
    else
        s->pista[0] = '\0';
    s->pista[MAX_PISTA - 1] = '\0';
    s->esq = s->dir = NULL;
    return s;
}

/* libera toda a arvore de salas (pos-ordem) */
void liberarArvoreSalas(Sala *raiz) {
    if (!raiz) return;
    liberarArvoreSalas(raiz->esq);
    liberarArvoreSalas(raiz->dir);
    free(raiz);
}

/* ===========================
   FUNCOES DA BST DE PISTAS
   =========================== */

/*
 * inserirPista() – insere a pista coletada na BST de forma ordenada.
 * Ignora duplicatas (nao insere duas vezes a mesma pista).
 * Retorna a raiz atualizada.
 */
PistaNode* inserirPista(PistaNode *raiz, const char *pista) {
    if (pista == NULL || pista[0] == '\0') return raiz; /* nada a inserir */

    if (raiz == NULL) {
        PistaNode *novo = (PistaNode*) malloc(sizeof(PistaNode));
        if (!novo) {
            fprintf(stderr, "Erro: falha ao alocar memoria para PistaNode\n");
            exit(1);
        }
        strncpy(novo->pista, pista, MAX_PISTA - 1);
        novo->pista[MAX_PISTA - 1] = '\0';
        novo->esq = novo->dir = NULL;
        return novo;
    }

    int cmp = strcmp(pista, raiz->pista);
    if (cmp < 0) {
        raiz->esq = inserirPista(raiz->esq, pista);
    } else if (cmp > 0) {
        raiz->dir = inserirPista(raiz->dir, pista);
    } else {
        /* duplicata: nao insere novamente */
    }
    return raiz;
}

/* exibe as pistas da BST em ordem (alfabetica) */
void exibirPistas(PistaNode *raiz) {
    if (!raiz) return;
    exibirPistas(raiz->esq);
    printf(" - %s\n", raiz->pista);
    exibirPistas(raiz->dir);
}

/* liberta BST de pistas (pos-ordem) */
void liberarArvorePistas(PistaNode *raiz) {
    if (!raiz) return;
    liberarArvorePistas(raiz->esq);
    liberarArvorePistas(raiz->dir);
    free(raiz);
}

/* percorre a BST e para cada pista chama uma funcao callback(pista, ctx)
   usada para contagem por suspeito posteriormente */
typedef void (*PistaCallback)(const char *pista, void *ctx);

void bst_traverse_inorder(PistaNode *raiz, PistaCallback cb, void *ctx) {
    if (!raiz) return;
    bst_traverse_inorder(raiz->esq, cb, ctx);
    cb(raiz->pista, ctx);
    bst_traverse_inorder(raiz->dir, cb, ctx);
}

/* ===========================
   FUNCOES DA HASH (pista -> suspeito)
   =========================== */

/* hash simples para strings: djb2 */
static unsigned int hash_string(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return (unsigned int)(hash % HASH_TABLE_SIZE);
}

/*
 * inserirNaHash() – insere associacao pista -> suspeito na tabela hash.
 * Se ja existir a pista, sobrescreve o suspeito (nao deveria ocorrer no uso normal).
 */
void inserirNaHash(HashTable *ht, const char *pista, const char *suspeito) {
    if (!pista || pista[0] == '\0') return;
    unsigned int idx = hash_string(pista);
    HashEntry *cur = ht->buckets[idx];
    while (cur) {
        if (strcmp(cur->pista, pista) == 0) {
            /* sobrescrever suspeito */
            strncpy(cur->suspeito, suspeito, MAX_NOME - 1);
            cur->suspeito[MAX_NOME - 1] = '\0';
            return;
        }
        cur = cur->prox;
    }
    /* nao encontrado: insere novo entry no inicio do bucket */
    HashEntry *novo = (HashEntry*) malloc(sizeof(HashEntry));
    if (!novo) {
        fprintf(stderr, "Erro: falha ao alocar HashEntry\n");
        exit(1);
    }
    strncpy(novo->pista, pista, MAX_PISTA - 1);
    novo->pista[MAX_PISTA - 1] = '\0';
    strncpy(novo->suspeito, suspeito, MAX_NOME - 1);
    novo->suspeito[MAX_NOME - 1] = '\0';
    novo->prox = ht->buckets[idx];
    ht->buckets[idx] = novo;
}

/*
 * encontrarSuspeito() – consulta a tabela hash usando a pista como chave.
 * Retorna ponteiro para o nome do suspeito (string interna) ou NULL se nao achar.
 */
const char* encontrarSuspeito(HashTable *ht, const char *pista) {
    if (!pista || pista[0] == '\0') return NULL;
    unsigned int idx = hash_string(pista);
    HashEntry *cur = ht->buckets[idx];
    while (cur) {
        if (strcmp(cur->pista, pista) == 0)
            return cur->suspeito;
        cur = cur->prox;
    }
    return NULL;
}

/* libera toda a tabela hash */
void liberarHash(HashTable *ht) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        HashEntry *cur = ht->buckets[i];
        while (cur) {
            HashEntry *tmp = cur;
            cur = cur->prox;
            free(tmp);
        }
        ht->buckets[i] = NULL;
    }
}

/* inicializa a hash (zera buckets) */
void initHash(HashTable *ht) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) ht->buckets[i] = NULL;
}

/* ===========================
   FUNCOES DE EXPLORACAO E JULGAMENTO
   =========================== */

/*
 * explorarSalas() – navega pela arvore interativamente a partir de 'raiz'.
 * Para cada sala visitada, exibe a pista (se houver) e adiciona a BST de pistas.
 */
void explorarSalas(Sala *raiz, PistaNode **arvorePistas) {
    if (!raiz) return;

    Sala *atual = raiz;
    char linha[32];

    printf("Iniciando exploracao (comandos: e = esquerda, d = direita, s = sair)\n");

    while (atual) {
        printf("\nVoce entrou em: %s\n", atual->nome);
        if (atual->pista[0] != '\0') {
            printf("Pista encontrada: \"%s\"\n", atual->pista);
            *arvorePistas = inserirPista(*arvorePistas, atual->pista);
        } else {
            printf("Nenhuma pista nesta sala.\n");
        }

        /* se folha, apenas permitir voltar? No enunciado, nao tem voltar, apenas seguir ate sair
           entao oferecemos opcoes conforme filhos existentes */
        if (atual->esq == NULL && atual->dir == NULL) {
            printf("Esta sala nao possui caminhos (no folha). Voce pode sair (s) ou encerrar exploracao.\n");
            printf("Digite 's' para sair ou outra tecla para encerrar exploracao: ");
            if (!fgets(linha, sizeof(linha), stdin)) break;
            if (tolower((unsigned char)linha[0]) == 's') {
                printf("Exploracao encerrada pelo jogador.\n");
            }
            break;
        }

        printf("Escolha um caminho:\n");
        if (atual->esq) printf("  (e) esquerda -> %s\n", atual->esq->nome);
        else printf("  (e) esquerda -> (bloqueado)\n");
        if (atual->dir) printf("  (d) direita -> %s\n", atual->dir->nome);
        else printf("  (d) direita -> (bloqueado)\n");
        printf("  (s) sair da exploracao\n");
        printf("Opcao: ");
        if (!fgets(linha, sizeof(linha), stdin)) break;
        char c = tolower((unsigned char)linha[0]);
        if (c == 'e') {
            if (atual->esq) atual = atual->esq;
            else printf("Caminho a esquerda indisponivel.\n");
        } else if (c == 'd') {
            if (atual->dir) atual = atual->dir;
            else printf("Caminho a direita indisponivel.\n");
        } else if (c == 's') {
            printf("Exploracao encerrada pelo jogador.\n");
            break;
        } else {
            printf("Comando invalido. Digite 'e', 'd' ou 's'.\n");
        }
    }
}

/*
 * verificarSuspeitoFinal() – verifica se ha pelo menos duas pistas que apontam
 * para o suspeito indicado pelo jogador.
 * Retorna número de pistas que apontam para ele.
 * Observação: usa a BST de pistas para obter a lista de pistas coletadas e a hash para mapear cada pista ao suspeito.
 */

/* contexto usado durante a travessia da BST para contar as ocorrencias apontando ao acusado */
typedef struct {
    const char *acusado;
    HashTable *ht;
    int contador;
} ContadorContext;

void contador_callback(const char *pista, void *ctx_void) {
    ContadorContext *ctx = (ContadorContext*) ctx_void;
    const char *sus = encontrarSuspeito(ctx->ht, pista);
    if (sus && strcmp(sus, ctx->acusado) == 0) {
        ctx->contador++;
    }
}

int verificarSuspeitoFinal(PistaNode *arvorePistas, HashTable *ht, const char *acusado) {
    if (!arvorePistas) return 0;
    ContadorContext ctx;
    ctx.acusado = acusado;
    ctx.ht = ht;
    ctx.contador = 0;
    bst_traverse_inorder(arvorePistas, contador_callback, &ctx);
    return ctx.contador;
}

/* ===========================
   UTILITARIOS
   =========================== */

/* lista as pistas coletadas (in-order), ou mensagem se nao houver */
void mostrarPistasColetadas(PistaNode *arvorePistas) {
    printf("\n== Pistas coletadas ==\n");
    if (!arvorePistas) {
        printf("(Nenhuma pista coletada)\n");
        return;
    }
    exibirPistas(arvorePistas);
}

/* transforma uma string removendo newline no fim */
void chomp(char *s) {
    size_t L = strlen(s);
    if (L == 0) return;
    if (s[L-1] == '\n') s[L-1] = '\0';
}

/* ===========================
   MAIN - monta mapa, popula hash e executa fluxo
   =========================== */

int main(void) {
    /* construir mansao (mapa fixo) - as pistas estao embutidas nas salas */
    Sala *hall = criarSala("Hall de Entrada", "Pegadas sujas perto da janela");
    Sala *salaEstar = criarSala("Sala de Estar", "Retrato pendurado torto");
    Sala *cozinha = criarSala("Cozinha", "Vasilha quebrada no chao");
    Sala *biblioteca = criarSala("Biblioteca", "Livro com anotacoes na margem");
    Sala *escritorio = criarSala("Escritorio", "Caneta com tinta vermelha");
    Sala *quarto = criarSala("Quarto", "Fio de tecido azul");
    Sala *sotao = criarSala("Sotao", "Chave enferrujada");
    Sala *jardim = criarSala("Jardim", "Pegadas que levam ao portao");

    /* montar ligacoes (arvore binaria) */
    hall->esq = salaEstar;
    hall->dir = cozinha;

    salaEstar->esq = biblioteca;
    salaEstar->dir = escritorio;

    cozinha->esq = quarto;
    cozinha->dir = sotao;

    biblioteca->esq = NULL;
    library: ; /* empty label to avoid unused label? (not needed) */

    /* jardim nao ligado em exemplo, podemos colocar sotao->dir = jardim para ter mais salas */
    sotao->dir = jardim;

    /* inicializar BST de pistas e tabela hash (pista->suspeito) */
    PistaNode *arvorePistas = NULL;
    HashTable ht;
    initHash(&ht);

    /* popular hash com associacoes pista -> suspeito
       (essas ligacoes seriam definidas pelo designer do jogo) */
    inserirNaHash(&ht, "Pegadas sujas perto da janela", "Sr. Black");
    inserirNaHash(&ht, "Retrato pendurado torto", "Sra. White");
    inserirNaHash(&ht, "Vasilha quebrada no chao", "Jovem Green");
    inserirNaHash(&ht, "Livro com anotacoes na margem", "Prof. Plum");
    inserirNaHash(&ht, "Caneta com tinta vermelha", "Sra. White");
    inserirNaHash(&ht, "Fio de tecido azul", "Jovem Green");
    inserirNaHash(&ht, "Chave enferrujada", "Sr. Black");
    inserirNaHash(&ht, "Pegadas que levam ao portao", "Sr. Black");

    /* iniciar exploracao interativa */
    explorarSalas(hall, &arvorePistas);

    /* mostrar pistas coletadas em ordem alfabetica */
    mostrarPistasColetadas(arvorePistas);

    /* pedir ao jogador que acuse um suspeito */
    char input[MAX_NOME];
    printf("\nIndique o nome do suspeito a ser acusado (ex: 'Sr. Black'): ");
    if (!fgets(input, sizeof(input), stdin)) {
        printf("Entrada falhou. Encerrando.\n");
    } else {
        chomp(input);
        if (strlen(input) == 0) {
            printf("Nenhum suspeito indicado. Encerrando.\n");
        } else {
            /* verificar quantas pistas apontam para esse suspeito */
            int qtd = verificarSuspeitoFinal(arvorePistas, &ht, input);
            printf("\nPistas que apontam para '%s': %d\n", input, qtd);
            if (qtd >= 2) {
                printf("Resultado: Ha evidencias suficientes. Acusacao sustentada!\n");
            } else {
                printf("Resultado: Evidencias insuficientes. Acusacao fragil.\n");
            }
        }
    }

    /* liberar memoria */
    liberarArvoreSalas(hall);
    liberarArvorePistas(arvorePistas);
    liberarHash(&ht);

    printf("\nFim do jogo. Obrigado por jogar Detective Quest!\n");
    return 0;
}
