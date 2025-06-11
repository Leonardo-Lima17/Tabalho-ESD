#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#define ALPHABET_SIZE 128

typedef struct {
    char transaction_id[16];
    char timestamp[32];
    char sender_account[32];
    char receiver_account[32];
    float amount;
    char transaction_type[16];
    char merchant_category[32];
    char location[32];
    char device_used[16];
} Transaction;

typedef struct TrieNode {
    struct TrieNode* children[ALPHABET_SIZE];
    Transaction* transacao;
} TrieNode;

typedef struct Grupo {
    char chave[64];
    float soma;
    int quantidade;
    struct Grupo* prox;
} Grupo;

TrieNode* create_trie_node() {
    TrieNode* node = (TrieNode*)malloc(sizeof(TrieNode));
    for (int i = 0; i < ALPHABET_SIZE; i++) node->children[i] = NULL;
    node->transacao = NULL;
    return node;
}
//Inserção de uma nova transação
void insert_trie(TrieNode* root, Transaction t) {
    TrieNode* curr = root;
    for (int i = 0; t.transaction_id[i]; i++) {
        unsigned char c = t.transaction_id[i];
        if (!curr->children[c])
            curr->children[c] = create_trie_node();
        curr = curr->children[c];
    }
    curr->transacao = (Transaction*)malloc(sizeof(Transaction));
    *(curr->transacao) = t;
}
//Busca de transação por ID
Transaction* search_trie(TrieNode* root, const char* id) {
    TrieNode* curr = root;
    for (int i = 0; id[i]; i++) {
        unsigned char c = id[i];
        if (!curr->children[c]) return NULL;
        curr = curr->children[c];
    }
    return curr->transacao;
}
//Remoção de transação por ID
bool delete_trie(TrieNode* root, const char* id, int depth) {
    if (!root) return false;
    if (id[depth] == '\0') {
        if (root->transacao) {
            free(root->transacao);
            root->transacao = NULL;
            for (int i = 0; i < ALPHABET_SIZE; i++)
                if (root->children[i]) return false;
            return true;
        }
        return false;
    }
    unsigned char c = id[depth];
    if (delete_trie(root->children[c], id, depth + 1)) {
        free(root->children[c]);
        root->children[c] = NULL;
        for (int i = 0; i < ALPHABET_SIZE; i++)
            if (root->children[i]) return false;
        return !root->transacao;
    }
    return false;
}

void inserir_grupo(Grupo** lista, const char* chave, float valor) {
    Grupo* atual = *lista;
    while (atual) {
        if (strcmp(atual->chave, chave) == 0) {
            atual->soma += valor;
            atual->quantidade++;
            return;
        }
        atual = atual->prox;
    }
    Grupo* novo = (Grupo*)malloc(sizeof(Grupo));
    strcpy(novo->chave, chave);
    novo->soma = valor;
    novo->quantidade = 1;
    novo->prox = *lista;
    *lista = novo;
}
//Agrupamento dos dados por campo
void agrupar_feature_trie(TrieNode* root, Grupo** lista, int feature) {
    if (!root) return;
    if (root->transacao) {
        const char* chave = NULL;
        Transaction* t = root->transacao;
        switch (feature) {
            case 1: chave = t->transaction_type; break;
            case 2: chave = t->merchant_category; break;
            case 3: chave = t->location; break;
            case 4: chave = t->device_used; break;
            case 5: chave = t->sender_account; break;
            case 6: chave = t->receiver_account; break;
            default: chave = "Indefinido";
        }
        inserir_grupo(lista, chave, t->amount);
    }
    for (int i = 0; i < ALPHABET_SIZE; i++)
        if (root->children[i])
            agrupar_feature_trie(root->children[i], lista, feature);
}

void imprimir_grupos(const char* nome, Grupo* lista) {
    printf("\nAgrupamento por %s:\n", nome);
    while (lista) {
        printf("%s -> Total: %.2f | Quantidade: %d\n", lista->chave, lista->soma, lista->quantidade);
        lista = lista->prox;
    }
}

bool prever_fraude(Transaction* t) {
    return (t->amount > 10000 || strcmp(t->device_used, "unknown") == 0);
}
//Leitura do Dataset
TrieNode* load_csv_trie(const char* filename) {
    TrieNode* root = create_trie_node();
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir o arquivo");
        exit(1);
    }
    char line[512];
    fgets(line, sizeof(line), file);
    while (fgets(line, sizeof(line), file)) {
        Transaction t;
        sscanf(line, "%15[^,],%31[^,],%31[^,],%31[^,],%f,%15[^,],%31[^,],%31[^,],%15[^,],%*s",
               t.transaction_id, t.timestamp, t.sender_account, t.receiver_account, &t.amount,
               t.transaction_type, t.merchant_category, t.location, t.device_used);
        insert_trie(root, t);
    }
    fclose(file);
    return root;
}

void remover_quebra(char* s) {
    s[strcspn(s, "\n")] = '\0';
}
typedef struct {
    float soma;
    float soma_quadrados;
    float min;
    float max;
    int count;
} Estatisticas;
//Cálculos estatísticos
void calcular_estatisticas_trie(TrieNode* root, Estatisticas* est) {
    if (!root) return;
    if (root->transacao) {
        float valor = root->transacao->amount;
        est->soma += valor;
        est->soma_quadrados += valor * valor;
        est->count++;
        if (valor < est->min) est->min = valor;
        if (valor > est->max) est->max = valor;
    }
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        if (root->children[i])
            calcular_estatisticas_trie(root->children[i], est);
    }
}

void exibir_estatisticas_trie(TrieNode* root) {
    Estatisticas est = {0, 0, INFINITY, -INFINITY, 0};
    calcular_estatisticas_trie(root, &est);
    if (est.count == 0) {
        printf("Nenhuma transacao registrada.\n");
        return;
    }
    float media = est.soma / est.count;
    float variancia = (est.soma_quadrados / est.count) - (media * media);
    float desvio_padrao = sqrtf(variancia);
    printf("\n=== Estatisticas das Transacoes ===\n");
    printf("Quantidade: %d\n", est.count);
    printf("Soma total: %.2f\n", est.soma);
    printf("Media: %.2f\n", media);
    printf("Desvio padrao: %.2f\n", desvio_padrao);
    printf("Minimo: %.2f\n", est.min);
    printf("Maximo: %.2f\n", est.max);
}

typedef struct {
    Transaction** lista;
    int tamanho;
    int capacidade;
} ListaTransacoes;

void inicializar_lista(ListaTransacoes* lt) {
    lt->capacidade = 1000;
    lt->tamanho = 0;
    lt->lista = (Transaction**)malloc(sizeof(Transaction*) * lt->capacidade);
}

void adicionar_transacao(ListaTransacoes* lt, Transaction* t) {
    if (lt->tamanho == lt->capacidade) {
        lt->capacidade *= 2;
        lt->lista = (Transaction**)realloc(lt->lista, sizeof(Transaction*) * lt->capacidade);
    }
    lt->lista[lt->tamanho++] = t;
}

void coletar_transacoes_trie(TrieNode* root, ListaTransacoes* lt) {
    if (!root) return;
    if (root->transacao)
        adicionar_transacao(lt, root->transacao);
    for (int i = 0; i < ALPHABET_SIZE; i++)
        if (root->children[i])
            coletar_transacoes_trie(root->children[i], lt);
}

bool corresponde_filtro(Transaction* t, int campo, const char* valor) {
    switch (campo) {
        case 1: return strcmp(t->transaction_type, valor) == 0;
        case 2: return strcmp(t->merchant_category, valor) == 0;
        case 3: return strcmp(t->location, valor) == 0;
        case 4: return strcmp(t->device_used, valor) == 0;
        case 5: return strcmp(t->sender_account, valor) == 0;
        case 6: return strcmp(t->receiver_account, valor) == 0;
        default: return true;
    }
}

int comparar_valor_crescente(const void* a, const void* b) {
    float va = (*(Transaction**)a)->amount;
    float vb = (*(Transaction**)b)->amount;
    return (va > vb) - (va < vb);
}

int comparar_valor_decrescente(const void* a, const void* b) {
    float va = (*(Transaction**)a)->amount;
    float vb = (*(Transaction**)b)->amount;
    return (vb > va) - (vb < va);
}

int comparar_data(const void* a, const void* b) {
    return strcmp((*(Transaction**)a)->timestamp, (*(Transaction**)b)->timestamp);
}

void exibir_transacao(Transaction* t) {
    printf("ID: %s\nTimestamp: %s\nSender: %s\nReceiver: %s\nValor: %.2f\nTipo: %s\nCategoria: %s\nLocal: %s\nDispositivo: %s\nPrev. Fraude: %s\n\n",
           t->transaction_id, t->timestamp, t->sender_account, t->receiver_account,
           t->amount, t->transaction_type, t->merchant_category,
           t->location, t->device_used, prever_fraude(t) ? "Sim" : "Nao");
}
//Filtragem e ordenação dos dados
void filtrar_e_ordenar_trie(TrieNode* root) {
    ListaTransacoes lt;
    inicializar_lista(&lt);
    coletar_transacoes_trie(root, &lt);

    if (lt.tamanho == 0) {
        printf("Nenhuma transacao encontrada.\n");
        return;
    }

    printf("Filtrar por:\n");
    printf("0. Nenhum filtro\n1. Tipo\n2. Categoria\n3. Local\n4. Dispositivo\n5. Sender\n6. Receiver\nEscolha: ");
    int campo; scanf("%d", &campo); getchar();

    char valor[64] = "";
    if (campo >= 1 && campo <= 6) {
        printf("Digite o valor a filtrar: ");
        fgets(valor, sizeof(valor), stdin); remover_quebra(valor);
    }

    ListaTransacoes filtradas;
    inicializar_lista(&filtradas);
    for (int i = 0; i < lt.tamanho; i++) {
        if (corresponde_filtro(lt.lista[i], campo, valor))
            adicionar_transacao(&filtradas, lt.lista[i]);
    }

    if (filtradas.tamanho == 0) {
        printf("Nenhuma transacao corresponde ao filtro.\n");
        free(lt.lista);
        free(filtradas.lista);
        return;
    }

    printf("Ordenar por:\n");
    printf("1. Valor (crescente)\n2. Valor (decrescente)\n3. Timestamp\nEscolha: ");
    int ordem; scanf("%d", &ordem); getchar();

    if (ordem == 1)
        qsort(filtradas.lista, filtradas.tamanho, sizeof(Transaction*), comparar_valor_crescente);
    else if (ordem == 2)
        qsort(filtradas.lista, filtradas.tamanho, sizeof(Transaction*), comparar_valor_decrescente);
    else if (ordem == 3)
        qsort(filtradas.lista, filtradas.tamanho, sizeof(Transaction*), comparar_data);

    printf("\n--- Transacoes Filtradas e Ordenadas ---\n");
    for (int i = 0; i < filtradas.tamanho; i++)
        exibir_transacao(filtradas.lista[i]);

    free(lt.lista);
    free(filtradas.lista);
}

int main() {
    TrieNode* root = load_csv_trie("C:\\Users\\leozi\\OneDrive\\Desktop\\TrabalhoESD.csv\\financial_fraud_detection_dataset.csv");
    int opcao;
    char id[16];

    do {
        printf("\n=== MENU TRIE ===\n");
        printf("1. Buscar\n2. Remover\n3. Inserir\n4. Agrupar\n5. Estatisticas\n6. Filtrar e Ordenar\n7. Sair\nOpcao: ");
        scanf("%d", &opcao); getchar();

        if (opcao == 1) {
            printf("ID: "); fgets(id, sizeof(id), stdin); remover_quebra(id);
            Transaction* t = search_trie(root, id);
            if (t)
                printf("ID: %s\nTimestamp: %s\nSender: %s\nReceiver: %s\nValor: %.2f\nTipo: %s\nCategoria: %s\nLocal: %s\nDispositivo: %s\nPrev. Fraude: %s\n",
                       t->transaction_id, t->timestamp, t->sender_account, t->receiver_account,
                       t->amount, t->transaction_type, t->merchant_category,
                       t->location, t->device_used, prever_fraude(t) ? "Sim" : "Nao");
            else
                printf("Nao encontrada.\n");

        } else if (opcao == 2) {
            printf("ID: "); fgets(id, sizeof(id), stdin); remover_quebra(id);
            delete_trie(root, id, 0);
            printf("Remocao concluida.\n");

        } else if (opcao == 3) {
            Transaction t;
            char temp[64];
            printf("ID: "); fgets(t.transaction_id, sizeof(t.transaction_id), stdin); remover_quebra(t.transaction_id);
            printf("Timestamp: "); fgets(t.timestamp, sizeof(t.timestamp), stdin); remover_quebra(t.timestamp);
            printf("Sender: "); fgets(t.sender_account, sizeof(t.sender_account), stdin); remover_quebra(t.sender_account);
            printf("Receiver: "); fgets(t.receiver_account, sizeof(t.receiver_account), stdin); remover_quebra(t.receiver_account);
            printf("Valor: "); fgets(temp, sizeof(temp), stdin); t.amount = atof(temp);
            printf("Tipo: "); fgets(t.transaction_type, sizeof(t.transaction_type), stdin); remover_quebra(t.transaction_type);
            printf("Categoria: "); fgets(t.merchant_category, sizeof(t.merchant_category), stdin); remover_quebra(t.merchant_category);
            printf("Local: "); fgets(t.location, sizeof(t.location), stdin); remover_quebra(t.location);
            printf("Dispositivo: "); fgets(t.device_used, sizeof(t.device_used), stdin); remover_quebra(t.device_used);
            insert_trie(root, t);
            printf("Inserida com sucesso.\n");

        } else if (opcao == 4) {
            printf("Agrupar por:\n1. Tipo\n2. Categoria\n3. Local\n4. Dispositivo\n5. Sender\n6. Receiver\nEscolha: ");
            int f; scanf("%d", &f); getchar();
            Grupo* grupos = NULL;
            agrupar_feature_trie(root, &grupos, f);
            const char* nomes[] = {"", "Tipo", "Categoria", "Local", "Dispositivo", "Sender", "Receiver"};
            imprimir_grupos(nomes[f], grupos);
        }
        else if (opcao == 5) {
           exibir_estatisticas_trie(root);
        }
        else if (opcao == 6) {
           filtrar_e_ordenar_trie(root);
        }


    } while (opcao != 7);

    return 0;
}
