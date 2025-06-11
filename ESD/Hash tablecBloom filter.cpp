#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define TABLE_SIZE 10007
#define BLOOM_SIZE 1000000

// === Bloom Filter ===
unsigned char bloom[BLOOM_SIZE];

unsigned int hash1(const char *str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash % BLOOM_SIZE;
}

unsigned int hash2(const char *str) {
    unsigned int hash = 0;
    int c;
    while ((c = *str++))
        hash = c + (hash << 6) + (hash << 16) - hash;
    return hash % BLOOM_SIZE;
}

void bloom_add(const char* str) {
    bloom[hash1(str)] = 1;
    bloom[hash2(str)] = 1;
}

bool bloom_check(const char* str) {
    return bloom[hash1(str)] && bloom[hash2(str)];
}

// === Hash Table Structures ===
typedef struct Transaction {
    char transaction_id[16];
    char timestamp[32];
    char sender_account[32];
    char receiver_account[32];
    float amount;
    char transaction_type[16];
    char merchant_category[32];
    char location[32];
    char device_used[16];
    bool is_fraud;
    struct Transaction* next;
} Transaction;

Transaction* hash_table[TABLE_SIZE];

unsigned int hash(const char* str) {
    unsigned int hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash % TABLE_SIZE;
}
//Inserção de transação 
void insert_transaction(Transaction t) {
    unsigned int index = hash(t.transaction_id);
    Transaction* new_node = (Transaction*)malloc(sizeof(Transaction));
    if (!new_node) {
        fprintf(stderr, "Erro de alocacao de memoria.\n");
        exit(1);
    }
    *new_node = t;
    new_node->next = hash_table[index];
    hash_table[index] = new_node;

    // Adiciona ao Bloom Filter
    bloom_add(t.transaction_id);
}
//Busca de transação por ID
Transaction* search_transaction(const char* transaction_id) {
    if (!bloom_check(transaction_id)) {
        return NULL; 
    }

    unsigned int index = hash(transaction_id);
    Transaction* current = hash_table[index];
    while (current) {
        if (strcmp(current->transaction_id, transaction_id) == 0)
            return current;
        current = current->next;
    }
    return NULL;
}
//Remoção de transação por ID
void remove_transaction(const char* transaction_id) {
    if (!bloom_check(transaction_id)) {
        printf("Transacao %s nao encontrada (Bloom Filter).\n", transaction_id);
        return;
    }

    unsigned int index = hash(transaction_id);
    Transaction* current = hash_table[index];
    Transaction* prev = NULL;

    while (current) {
        if (strcmp(current->transaction_id, transaction_id) == 0) {
            if (prev)
                prev->next = current->next;
            else
                hash_table[index] = current->next;
            free(current);
            printf("Transacao %s removida.\n", transaction_id);
            return;
        }
        prev = current;
        current = current->next;
    }
    printf("Transacao %s nao encontrada.\n", transaction_id);
}
//Leitura do Dataset
void load_csv(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir o arquivo");
        exit(1);
    }

    char line[512];
    fgets(line, sizeof(line), file); // Pula cabeçalho

    while (fgets(line, sizeof(line), file)) {
        Transaction t;
        char is_fraud_str[6];

        int campos_lidos = sscanf(line, "%15[^,],%31[^,],%31[^,],%31[^,],%f,%15[^,],%31[^,],%31[^,],%15[^,],%5[^,\n]",
               t.transaction_id, t.timestamp, t.sender_account, t.receiver_account, &t.amount,
               t.transaction_type, t.merchant_category, t.location, t.device_used, is_fraud_str);

        if (campos_lidos == 10) {
            t.is_fraud = strcmp(is_fraud_str, "True") == 0;
            insert_transaction(t);
        } else {
            fprintf(stderr, "Linha ignorada por formato invalido: %s", line);
        }
    }

    fclose(file);
}

int compare_floats(const void* a, const void* b) {
    float fa = *(const float*)a;
    float fb = *(const float*)b;
    return (fa > fb) - (fa < fb);
}
//Cálculos estatísticos
void calcular_estatisticas() {
    float* valores = NULL;
    int total = 0;
    float soma = 0, maior = 0, menor = -1;
    int total_fraudes = 0;

    int total_transacoes = 0;
    for (int i = 0; i < TABLE_SIZE; i++) {
        Transaction* t = hash_table[i];
        while (t) {
            total_transacoes++;
            t = t->next;
        }
    }

    if (total_transacoes == 0) {
        printf("Nenhuma transacao registrada.\n");
        return;
    }

    valores = (float*)malloc(total_transacoes * sizeof(float));
    if (!valores) {
        printf("Erro ao alocar memoria.\n");
        return;
    }

    for (int i = 0; i < TABLE_SIZE; i++) {
        Transaction* t = hash_table[i];
        while (t) {
            float v = t->amount;
            valores[total++] = v;
            soma += v;
            if (menor < 0 || v < menor) menor = v;
            if (v > maior) maior = v;
            if (t->is_fraud) total_fraudes++;
            t = t->next;
        }
    }

    float media = soma / total;
    float somatorio = 0;
    for (int j = 0; j < total; j++)
        somatorio += pow(valores[j] - media, 2);
    float desvio_padrao = sqrt(somatorio / total);

    qsort(valores, total, sizeof(float), compare_floats);
    float mediana = (total % 2 == 0)
        ? (valores[total/2 - 1] + valores[total/2]) / 2
        : valores[total/2];

    float moda = valores[0];
    int current_count = 1, max_count = 1;
    for (int i = 1; i < total; i++) {
        if (valores[i] == valores[i-1]) {
            current_count++;
            if (current_count > max_count) {
                max_count = current_count;
                moda = valores[i];
            }
        } else {
            current_count = 1;
        }
    }

    float porcentagem_fraudes = (total_fraudes * 100.0f) / total;

    printf("\n=== Estatisticas ===\n");
    printf("Total de transacoes: %d\n", total);
    printf("Total de fraudes: %d (%.2f%%)\n", total_fraudes, porcentagem_fraudes);
    printf("Valor total movimentado: %.2f\n", soma);
    printf("Media dos valores: %.2f\n", media);
    printf("Desvio padrao: %.2f\n", desvio_padrao);
    printf("Maior valor: %.2f\n", maior);
    printf("Menor valor: %.2f\n", menor);
    printf("Mediana: %.2f\n", mediana);
    printf("Moda: %.2f (%d ocorrencias)\n", moda, max_count);

    free(valores);
}

// Agrupamento dos dados
typedef struct Grupo {
    char chave[64];
    int quantidade;
    float soma_valores;
    struct Grupo* prox;
} Grupo;

void inserir_grupo(Grupo** lista, const char* chave, float valor) {
    Grupo* atual = *lista;
    while (atual) {
        if (strcmp(atual->chave, chave) == 0) {
            atual->quantidade++;
            atual->soma_valores += valor;
            return;
        }
        atual = atual->prox;
    }

    Grupo* novo = (Grupo*)malloc(sizeof(Grupo));
    if (!novo) {
        printf("Erro ao alocar memoria para grupo.\n");
        return;
    }

    strcpy(novo->chave, chave);
    novo->quantidade = 1;
    novo->soma_valores = valor;
    novo->prox = *lista;
    *lista = novo;
}

void agrupar_por_campo(int campo) {
    Grupo* grupos = NULL;
    const char* titulo = "";

    for (int i = 0; i < TABLE_SIZE; i++) {
        Transaction* t = hash_table[i];
        while (t) {
            const char* chave = "";
            switch (campo) {
                case 1: chave = t->transaction_type; titulo = "Tipo de Transacao"; break;
                case 2: chave = t->merchant_category; titulo = "Categoria do Comerciante"; break;
                case 3: chave = t->location; titulo = "Localizacao"; break;
                case 4: chave = t->device_used; titulo = "Dispositivo Usado"; break;
                case 5: chave = t->sender_account; titulo = "Conta do Remetente"; break;
                case 6: chave = t->receiver_account; titulo = "Conta do Destinatario"; break;
                default: chave = "Indefinido"; titulo = "Indefinido";
            }

            inserir_grupo(&grupos, chave, t->amount);
            t = t->next;
        }
    }

    printf("\n=== Agrupamento por %s ===\n", titulo);
    printf("%-30s | %-8s | %-12s | %-10s\n", "Campo", "Qtd", "Total", "Media");
    printf("------------------------------------------------------------\n");

    Grupo* atual = grupos;
    while (atual) {
        printf("%-30s | %-8d | %-12.2f | %-10.2f\n",
               atual->chave,
               atual->quantidade,
               atual->soma_valores,
               atual->soma_valores / atual->quantidade);
        atual = atual->prox;
    }

    while (grupos) {
        Grupo* temp = grupos;
        grupos = grupos->prox;
        free(temp);
    }
}
//Filtragem dos dados 
void filtrar_transacoes() {
    int opcao;
    printf("\n=== FILTRAR TRANSACOES ===\n");
    printf("1. Apenas fraudes\n");
    printf("2. Valor acima de um limite\n");
    printf("3. Valor abaixo de um limite\n");
    printf("4. Por sender_account\n");
    printf("Escolha uma opcao: ");
    scanf("%d", &opcao);
    getchar();

    float limite = 0;
    char conta[32] = "";

    // Coleta do valor ou conta apenas uma vez, antes de iterar
    switch (opcao) {
        case 2:
            printf("Digite o valor minimo: ");
            scanf("%f", &limite);
            getchar();
            break;
        case 3:
            printf("Digite o valor maximo: ");
            scanf("%f", &limite);
            getchar();
            break;
        case 4:
            printf("Digite o sender_account: ");
            fgets(conta, sizeof(conta), stdin);
            conta[strcspn(conta, "\n")] = '\0'; // remove newline
            break;
    }

    for (int i = 0; i < TABLE_SIZE; i++) {
        Transaction* t = hash_table[i];
        while (t) {
            bool exibir = false;

            switch (opcao) {
                case 1:
                    exibir = t->is_fraud;
                    break;
                case 2:
                    exibir = t->amount >= limite;
                    break;
                case 3:
                    exibir = t->amount <= limite;
                    break;
                case 4:
                    exibir = strcmp(t->sender_account, conta) == 0;
                    break;
                default:
                    printf("Opcao invalida!\n");
                    return;
            }

            if (exibir) {
                printf("\nID: %s | Valor: %.2f | Fraude: %s\n", t->transaction_id, t->amount, t->is_fraud ? "Sim" : "Nao");
            }

            t = t->next;
        }
    }
}
int compare_transactions(const void* a, const void* b) {
    Transaction* t1 = *(Transaction**)a;
    Transaction* t2 = *(Transaction**)b;
    return (t1->amount > t2->amount) - (t1->amount < t2->amount);
}
//Ordenação dos dados
void ordenar_transacoes() {
    int total = 0;

    // Contar quantas transações há
    for (int i = 0; i < TABLE_SIZE; i++) {
        Transaction* t = hash_table[i];
        while (t) {
            total++;
            t = t->next;
        }
    }

    if (total == 0) {
        printf("Nenhuma transacao encontrada.\n");
        return;
    }

    // Armazenar ponteiros em array
    Transaction** lista = (Transaction**)malloc(total * sizeof(Transaction*));
    int idx = 0;

    for (int i = 0; i < TABLE_SIZE; i++) {
        Transaction* t = hash_table[i];
        while (t) {
            lista[idx++] = t;
            t = t->next;
        }
    }

    qsort(lista, total, sizeof(Transaction*), compare_transactions);

    printf("\n=== TRANSACOES ORDENADAS POR VALOR ===\n");
    for (int i = 0; i < total; i++) {
        printf("ID: %s | Valor: %.2f | Fraude: %s\n", 
               lista[i]->transaction_id, lista[i]->amount, lista[i]->is_fraud ? "Sim" : "Nao");
    }

    free(lista);
}
// === MAIN ===
int main() {
    load_csv("C:\\Users\\leozi\\OneDrive\\Desktop\\TrabalhoESD.csv\\financial_fraud_detection_dataset.csv");

    int opcao;
    char id[16];

    do {
        printf("\n=== MENU ===\n");
        printf("1. Buscar transacao\n");
        printf("2. Remover transacao\n");
        printf("3. Inserir nova transacao\n");
        printf("4. Sair\n");
        printf("5. Mostrar estatisticas\n");
        printf("6. Agrupar por campo\n");
        printf("7. Filtrar transacoes\n");
        printf("8. Ordenar transacoes por valor\n");
        printf("Escolha uma opcao: ");
        scanf("%d", &opcao);
        getchar();

        switch (opcao) {
            case 1:
                printf("Digite o transaction_id para buscar: ");
                fgets(id, sizeof(id), stdin);
                id[strcspn(id, "\n")] = '\0';
                {
                    Transaction* t = search_transaction(id);
                    if (t) {
                        printf("Transacao encontrada:\n");
                        printf("ID: %s\n", t->transaction_id);
                        printf("Data/Hora: %s\n", t->timestamp);
                        printf("De: %s | Para: %s\n", t->sender_account, t->receiver_account);
                        printf("Valor: %.2f | Fraude: %s\n", t->amount, t->is_fraud ? "Sim" : "Nao");
                        printf("Tipo: %s | Categoria: %s\n", t->transaction_type, t->merchant_category);
                        printf("Local: %s | Dispositivo: %s\n", t->location, t->device_used);
                    } else {
                        printf("Transacao nao encontrada.\n");
                    }
                }
                break;

            case 2:
                printf("Digite o transaction_id para remover: ");
                fgets(id, sizeof(id), stdin);
                id[strcspn(id, "\n")] = '\0';
                remove_transaction(id);
                break;

            case 3: {
                Transaction nova;
                char is_fraud_str[6];

                printf("Digite o transaction_id: ");
                fgets(nova.transaction_id, sizeof(nova.transaction_id), stdin);
                nova.transaction_id[strcspn(nova.transaction_id, "\n")] = '\0';

                printf("Digite o timestamp: ");
                fgets(nova.timestamp, sizeof(nova.timestamp), stdin);
                nova.timestamp[strcspn(nova.timestamp, "\n")] = '\0';

                printf("Digite o sender_account: ");
                fgets(nova.sender_account, sizeof(nova.sender_account), stdin);
                nova.sender_account[strcspn(nova.sender_account, "\n")] = '\0';

                printf("Digite o receiver_account: ");
                fgets(nova.receiver_account, sizeof(nova.receiver_account), stdin);
                nova.receiver_account[strcspn(nova.receiver_account, "\n")] = '\0';

                printf("Digite o valor (float): ");
                scanf("%f", &nova.amount);
                getchar();

                printf("Digite o transaction_type: ");
                fgets(nova.transaction_type, sizeof(nova.transaction_type), stdin);
                nova.transaction_type[strcspn(nova.transaction_type, "\n")] = '\0';

                printf("Digite o merchant_category: ");
                fgets(nova.merchant_category, sizeof(nova.merchant_category), stdin);
                nova.merchant_category[strcspn(nova.merchant_category, "\n")] = '\0';

                printf("Digite o location: ");
                fgets(nova.location, sizeof(nova.location), stdin);
                nova.location[strcspn(nova.location, "\n")] = '\0';

                printf("Digite o device_used: ");
                fgets(nova.device_used, sizeof(nova.device_used), stdin);
                nova.device_used[strcspn(nova.device_used, "\n")] = '\0';

                printf("E fraude? (True/False): ");
                fgets(is_fraud_str, sizeof(is_fraud_str), stdin);
                is_fraud_str[strcspn(is_fraud_str, "\n")] = '\0';
                nova.is_fraud = (strcmp(is_fraud_str, "True") == 0);

                insert_transaction(nova);
                printf("Transacao inserida com sucesso!\n");
                break;
            }

            case 4:
                printf("Encerrando...\n");
                break;

            case 5:
                calcular_estatisticas();
                break;

            case 6: {
                printf("\nAgrupar por:\n");
                printf("1. Tipo de Transacao\n");
                printf("2. Categoria do Comerciante\n");
                printf("3. Localizacao\n");
                printf("4. Dispositivo Usado\n");
                printf("5. Conta do Remetente\n");
                printf("6. Conta do Destinatario\n");
                printf("Escolha o campo: ");
                int campo;
                scanf("%d", &campo);
                getchar();

                if (campo >= 1 && campo <= 6) {
                    agrupar_por_campo(campo);
                } else {
                    printf("Opcao invalida!\n");
                }
                break;
            }
            case 7:
            filtrar_transacoes();
            break;

            case 8:
            ordenar_transacoes();
            break;

            default:
                printf("Opcao invalida.\n");
        }
    } while (opcao != 4);

    return 0;
}