#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TAM_FILA 5
#define TAM_PILHA 3

// Estrutura que representa uma peça do Tetris
typedef struct {
    char tipo; // 'I', 'O', 'T', 'L'
    int id;    // identificador único
} Peca;

// ---------------------- FILA CIRCULAR ----------------------
typedef struct {
    Peca pecas[TAM_FILA];
    int frente;
    int tras;
    int quantidade;
} Fila;

// Inicializa a fila
void inicializarFila(Fila *f) {
    f->frente = 0;
    f->tras = -1;
    f->quantidade = 0;
}

// Verifica se a fila está cheia
int filaCheia(Fila *f) {
    return f->quantidade == TAM_FILA;
}

// Verifica se a fila está vazia
int filaVazia(Fila *f) {
    return f->quantidade == 0;
}

// Adiciona uma peça à fila (enqueue)
void enfileirar(Fila *f, Peca p) {
    if (filaCheia(f)) return;
    f->tras = (f->tras + 1) % TAM_FILA;
    f->pecas[f->tras] = p;
    f->quantidade++;
}

// Remove uma peça da fila (dequeue)
Peca desenfileirar(Fila *f) {
    Peca vazia = {'-', -1};
    if (filaVazia(f)) return vazia;
    Peca p = f->pecas[f->frente];
    f->frente = (f->frente + 1) % TAM_FILA;
    f->quantidade--;
    return p;
}

// ---------------------- PILHA ----------------------
typedef struct {
    Peca pecas[TAM_PILHA];
    int topo;
} Pilha;

// Inicializa a pilha
void inicializarPilha(Pilha *p) {
    p->topo = -1;
}

// Verifica se a pilha está cheia
int pilhaCheia(Pilha *p) {
    return p->topo == TAM_PILHA - 1;
}

// Verifica se a pilha está vazia
int pilhaVazia(Pilha *p) {
    return p->topo == -1;
}

// Empilha (push)
void empilhar(Pilha *p, Peca nova) {
    if (pilhaCheia(p)) return;
    p->pecas[++p->topo] = nova;
}

// Desempilha (pop)
Peca desempilhar(Pilha *p) {
    Peca vazia = {'-', -1};
    if (pilhaVazia(p)) return vazia;
    return p->pecas[p->topo--];
}

// ---------------------- FUNÇÕES AUXILIARES ----------------------

// Gera uma nova peça com ID único e tipo aleatório
Peca gerarPeca(int id) {
    char tipos[] = {'I', 'O', 'T', 'L'};
    Peca nova;
    nova.tipo = tipos[rand() % 4];
    nova.id = id;
    return nova;
}

// Exibe o estado atual da fila e da pilha
void exibirEstado(Fila *f, Pilha *p) {
    printf("\n-----------------------------\n");
    printf("Fila de pecas futuras:\n");
    for (int i = 0; i < f->quantidade; i++) {
        int idx = (f->frente + i) % TAM_FILA;
        printf("[%c %d] ", f->pecas[idx].tipo, f->pecas[idx].id);
    }
    printf("\n-----------------------------\n");

    printf("Pilha de reserva (Topo -> Base):\n");
    if (pilhaVazia(p))
        printf("(vazia)\n");
    else {
        for (int i = p->topo; i >= 0; i--) {
            printf("[%c %d] ", p->pecas[i].tipo, p->pecas[i].id);
        }
        printf("\n");
    }
    printf("-----------------------------\n");
}

// Troca simples entre o topo da pilha e a frente da fila
void trocarTopoComFrente(Fila *fila, Pilha *pilha) {
    if (pilhaVazia(pilha) || filaVazia(fila)) {
        printf("Nao e possivel trocar. Uma das estruturas esta vazia.\n");
        return;
    }

    int frente = fila->frente;
    int topo = pilha->topo;

    Peca temp = fila->pecas[frente];
    fila->pecas[frente] = pilha->pecas[topo];
    pilha->pecas[topo] = temp;

    printf("Troca realizada entre a frente da fila e o topo da pilha!\n");
}

// Troca múltipla (3 da fila <-> 3 da pilha)
void trocaMultipla(Fila *fila, Pilha *pilha) {
    if (fila->quantidade < 3 || pilha->topo < 2) {
        printf("Nao e possivel realizar troca multipla (faltam pecas).\n");
        return;
    }

    for (int i = 0; i < 3; i++) {
        int idxFila = (fila->frente + i) % TAM_FILA;
        int idxPilha = pilha->topo - i;

        Peca temp = fila->pecas[idxFila];
        fila->pecas[idxFila] = pilha->pecas[idxPilha];
        pilha->pecas[idxPilha] = temp;
    }

    printf("Troca multipla entre as 3 primeiras pecas da fila e da pilha concluida!\n");
}

// ---------------------- MAIN ----------------------
int main() {
    srand(time(NULL));

    Fila fila;
    Pilha pilha;
    inicializarFila(&fila);
    inicializarPilha(&pilha);

    int idCounter = 0;

    // Inicializa a fila com 5 peças
    for (int i = 0; i < TAM_FILA; i++) {
        enfileirar(&fila, gerarPeca(idCounter++));
    }

    int opcao;
    do {
        exibirEstado(&fila, &pilha);
        printf("\nOpcoes disponiveis:\n");
        printf("1 - Jogar peca (remover da fila)\n");
        printf("2 - Reservar peca (mover para pilha)\n");
        printf("3 - Usar peca reservada (remover do topo da pilha)\n");
        printf("4 - Trocar peca da frente com o topo da pilha\n");
        printf("5 - Trocar as 3 primeiras da fila com as 3 da pilha\n");
        printf("0 - Sair\n");
        printf("Opcao: ");
        scanf("%d", &opcao);

        switch (opcao) {
            case 1: { // Jogar peça
                if (!filaVazia(&fila)) {
                    Peca jogada = desenfileirar(&fila);
                    printf("Peca [%c %d] jogada!\n", jogada.tipo, jogada.id);
                    enfileirar(&fila, gerarPeca(idCounter++));
                } else {
                    printf("Fila vazia!\n");
                }
                break;
            }
            case 2: { // Reservar peça
                if (pilhaCheia(&pilha)) {
                    printf("Pilha cheia! Nao e possivel reservar mais pecas.\n");
                } else if (!filaVazia(&fila)) {
                    Peca reservada = desenfileirar(&fila);
                    empilhar(&pilha, reservada);
                    printf("Peca [%c %d] movida para a reserva!\n", reservada.tipo, reservada.id);
                    enfileirar(&fila, gerarPeca(idCounter++));
                }
                break;
            }
            case 3: { // Usar peça da reserva
                if (!pilhaVazia(&pilha)) {
                    Peca usada = desempilhar(&pilha);
                    printf("Peca reservada [%c %d] usada!\n", usada.tipo, usada.id);
                } else {
                    printf("Pilha de reserva vazia!\n");
                }
                break;
            }
            case 4: // Troca simples
                trocarTopoComFrente(&fila, &pilha);
                break;

            case 5: // Troca múltipla
                trocaMultipla(&fila, &pilha);
                break;

            case 0:
                printf("Encerrando o programa...\n");
                break;

            default:
                printf("Opcao invalida!\n");
        }

    } while (opcao != 0);

    return 0;
}