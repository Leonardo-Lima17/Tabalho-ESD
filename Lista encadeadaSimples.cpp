#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

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

typedef struct Grupo {
    char chave[64];
    int quantidade;
    float soma_valores;
    struct Grupo* prox;
} Grupo;

Transaction* head = NULL;

// Funções básicas da lista encadeada
//Inserção de uma nova transação
void insert_transaction(Transaction t) {
    Transaction* new_node = (Transaction*)malloc(sizeof(Transaction));
    if (!new_node) {
        fprintf(stderr, "Erro de alocacao de memoria.\n");
        exit(1);
    }
    *new_node = t;
    new_node->next = head;
    head = new_node;
}
//Busca por ID de transação
Transaction* search_transaction(const char* transaction_id) {
    Transaction* current = head;
    while (current) {
        if (strcmp(current->transaction_id, transaction_id) == 0)
            return current;
        current = current->next;
    }
    return NULL;
}
//Remoção por ID de transação
void remove_transaction(const char* transaction_id) {
    Transaction* current = head;
    Transaction* prev = NULL;

    while (current) {
        if (strcmp(current->transaction_id, transaction_id) == 0) {
            if (prev)
                prev->next = current->next;
            else
                head = current->next;
            free(current);
            printf("Transacao %s removida.\n", transaction_id);
            return;
        }
        prev = current;
        current = current->next;
    }
    printf("Transacao %s nao encontrada.\n", transaction_id);
}

// Função para contar transações
int contar_transacoes() {
    int count = 0;
    Transaction* current = head;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

// Função para coletar dados para estatísticas
void coletar_dados(float* valores, int* index, int* total_fraudes, 
                  float* soma, float* maior, float* menor) {
    Transaction* current = head;
    while (current) {
        valores[*index] = current->amount;
        (*index)++;
        *soma += current->amount;
        
        if (current->amount > *maior) *maior = current->amount;
        if (current->amount < *menor) *menor = current->amount;
        if (current->is_fraud) (*total_fraudes)++;
        
        current = current->next;
    }
}

// Função de comparação para qsort
int compare_floats(const void* a, const void* b) {
    float fa = *(const float*)a;
    float fb = *(const float*)b;
    return (fa > fb) - (fa < fb);
}

// Estatísticas completas
void calcular_estatisticas() {
    int total = contar_transacoes();
    if (total == 0) {
        printf("Nenhuma transacao registrada.\n");
        return;
    }

    float* valores = (float*)malloc(total * sizeof(float));
    if (!valores) {
        printf("Erro ao alocar memoria.\n");
        return;
    }

    int index = 0, total_fraudes = 0;
    float soma = 0, maior = -INFINITY, menor = INFINITY;
    
    coletar_dados(valores, &index, &total_fraudes, &soma, &maior, &menor);
    
    // Ordenar para mediana e moda
    qsort(valores, total, sizeof(float), compare_floats);

    // Calcular mediana
    float mediana = (total % 2 == 0) ? 
                   (valores[total/2 - 1] + valores[total/2]) / 2 : 
                   valores[total/2];

    // Calcular moda
    float moda = valores[0];
    int count = 1, max_count = 1;
    for (int i = 1; i < total; i++) {
        if (valores[i] == valores[i-1]) {
            count++;
            if (count > max_count) {
                max_count = count;
                moda = valores[i];
            }
        } else {
            count = 1;
        }
    }

    // Calcular média e desvio padrão
    float media = soma / total;
    float variancia = 0;
    for (int i = 0; i < total; i++) {
        variancia += pow(valores[i] - media, 2);
    }
    float desvio = sqrt(variancia / total);
    float porcentagem_fraudes = (total_fraudes * 100.0f) / total;

    printf("\n=== Estatisticas ===\n");
    printf("Total de transacoes: %d\n", total);
    printf("Total de fraudes: %d (%.2f%%)\n", total_fraudes, porcentagem_fraudes);
    printf("Valor total movimentado: %.2f\n", soma);
    printf("Media dos valores: %.2f\n", media);
    printf("Desvio padrao: %.2f\n", desvio);
    printf("Maior valor: %.2f\n", maior);
    printf("Menor valor: %.2f\n", menor);
    printf("Mediana: %.2f\n", mediana);
    printf("Moda: %.2f (%d ocorrencias)\n", moda, max_count);

    free(valores);
}

// Funções para agrupamento
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
    
    Transaction* current = head;
    while (current) {
        const char* chave = "";
        switch (campo) {
            case 1: chave = current->transaction_type; titulo = "Tipo de Transacao"; break;
            case 2: chave = current->merchant_category; titulo = "Categoria do Comerciante"; break;
            case 3: chave = current->location; titulo = "Localizacao"; break;
            case 4: chave = current->device_used; titulo = "Dispositivo Usado"; break;
            case 5: chave = current->sender_account; titulo = "Conta do Remetente"; break;
            case 6: chave = current->receiver_account; titulo = "Conta do Destinatario"; break;
            default: chave = "Indefinido"; titulo = "Indefinido";
        }
        
        inserir_grupo(&grupos, chave, current->amount);
        current = current->next;
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
    
    // Liberar memória
    while (grupos) {
        Grupo* temp = grupos;
        grupos = grupos->prox;
        free(temp);
    }
}

// Carregar Dataset
void load_csv(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir o arquivo");
        exit(1);
    }

    char line[512];
    fgets(line, sizeof(line), file); // pula cabeçalho

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
            fprintf(stderr, "Linha ignorada por formato inválido: %s", line);
        }
    }

    fclose(file);
}

// Função auxiliar para previsão de fraude
bool prever_fraude(Transaction* t) {
    return (t->amount > 10000 || strcmp(t->device_used, "unknown") == 0);
}
//Filtragem dos dados
void filtrar_transacoes() {
    int opcao;
    float limite;
    char tipo[16];

    printf("\n=== Filtrar Transacoes ===\n");
    printf("1. Valor maior que\n");
    printf("2. Valor menor que\n");
    printf("3. Por tipo de transacao\n");
    printf("Escolha uma opcao: ");
    scanf("%d", &opcao);
    getchar();

    Transaction* current = head;
    switch (opcao) {
        case 1:
            printf("Digite o valor minimo: ");
            scanf("%f", &limite);
            getchar();
            while (current) {
                if (current->amount > limite) {
                    printf("%s | %.2f | %s\n", current->transaction_id, current->amount, current->transaction_type);
                }
                current = current->next;
            }
            break;
        case 2:
            printf("Digite o valor maximo: ");
            scanf("%f", &limite);
            getchar();
            while (current) {
                if (current->amount < limite) {
                    printf("%s | %.2f | %s\n", current->transaction_id, current->amount, current->transaction_type);
                }
                current = current->next;
            }
            break;
        case 3:
            printf("Digite o tipo de transacao: ");
            fgets(tipo, sizeof(tipo), stdin);
            tipo[strcspn(tipo, "\n")] = '\0';
            while (current) {
                if (strcmp(current->transaction_type, tipo) == 0) {
                    printf("%s | %.2f | %s\n", current->transaction_id, current->amount, current->transaction_type);
                }
                current = current->next;
            }
            break;
        default:
            printf("Opcao invalida.\n");
    }
}

// Cria vetor auxiliar para ordenação
Transaction** listar_transacoes_em_array(int* total) {
    *total = contar_transacoes();
    if (*total == 0) return NULL;

    Transaction** vetor = (Transaction**)malloc(sizeof(Transaction*) * (*total));
    Transaction* current = head;
    for (int i = 0; i < *total && current; i++) {
        vetor[i] = current;
        current = current->next;
    }
    return vetor;
}

// Comparadores
int comparar_por_valor(const void* a, const void* b) {
    Transaction* ta = *(Transaction**)a;
    Transaction* tb = *(Transaction**)b;
    return (ta->amount > tb->amount) - (ta->amount < tb->amount);
}

int comparar_por_data(const void* a, const void* b) {
    Transaction* ta = *(Transaction**)a;
    Transaction* tb = *(Transaction**)b;
    return strcmp(ta->timestamp, tb->timestamp);
}

// Ordenação dos dados
void ordenar_transacoes() {
    int total = 0;
    Transaction** lista = listar_transacoes_em_array(&total);
    if (!lista) {
        printf("Nenhuma transacao para ordenar.\n");
        return;
    }

    int opcao;
    printf("\n=== Ordenar Transacoes ===\n");
    printf("1. Por valor (ascendente)\n");
    printf("2. Por data/hora (ascendente)\n");
    printf("Escolha uma opcao: ");
    scanf("%d", &opcao);
    getchar();

    if (opcao == 1) {
        qsort(lista, total, sizeof(Transaction*), comparar_por_valor);
    } else if (opcao == 2) {
        qsort(lista, total, sizeof(Transaction*), comparar_por_data);
    } else {
        printf("Opcao invalida.\n");
        free(lista);
        return;
    }

    printf("\nTransacoes ordenadas:\n");
    for (int i = 0; i < total; i++) {
        printf("%s | %.2f | %s\n", lista[i]->transaction_id, lista[i]->amount, lista[i]->timestamp);
    }

    free(lista);
}

// Menu principal
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
        printf("8. Ordenar transacoes\n");
        printf("Escolha uma opcao: ");
        scanf("%d", &opcao);
        getchar();

        switch (opcao) {
            case 1: {
                printf("Digite o transaction_id para buscar: ");
                fgets(id, sizeof(id), stdin);
                id[strcspn(id, "\n")] = '\0';
                
                Transaction* t = search_transaction(id);
                if (t) {
                    printf("\nTransacao encontrada:\n");
                    printf("ID: %s\n", t->transaction_id);
                    printf("Data/Hora: %s\n", t->timestamp);
                    printf("De: %s | Para: %s\n", t->sender_account, t->receiver_account);
                    printf("Valor: %.2f | Fraude: %s\n", t->amount, t->is_fraud ? "Sim" : "Nao");
                    printf("Tipo: %s | Categoria: %s\n", t->transaction_type, t->merchant_category);
                    printf("Local: %s | Dispositivo: %s\n", t->location, t->device_used);
                    printf("Previsao de Fraude: %s\n", prever_fraude(t) ? "Sim" : "Nao");
                } else {
                    printf("Transacao não encontrada.\n");
                }
                break;
            }
                
            case 2: {
                printf("Digite o transaction_id para remover: ");
                fgets(id, sizeof(id), stdin);
                id[strcspn(id, "\n")] = '\0';
                remove_transaction(id);
                break;
            }
                
            case 3: {
                Transaction nova;
                char is_fraud_str[6];

                printf("Digite o transaction_id: ");
                fgets(nova.transaction_id, sizeof(nova.transaction_id), stdin);
                printf("Digite o timestamp: ");
                fgets(nova.timestamp, sizeof(nova.timestamp), stdin);
                printf("Digite o sender_account: ");
                fgets(nova.sender_account, sizeof(nova.sender_account), stdin);
                printf("Digite o receiver_account: ");
                fgets(nova.receiver_account, sizeof(nova.receiver_account), stdin);
                printf("Digite o valor (float): ");
                scanf("%f", &nova.amount);
                getchar();
                printf("Digite o transaction_type: ");
                fgets(nova.transaction_type, sizeof(nova.transaction_type), stdin);
                printf("Digite o merchant_category: ");
                fgets(nova.merchant_category, sizeof(nova.merchant_category), stdin);
                printf("Digite o location: ");
                fgets(nova.location, sizeof(nova.location), stdin);
                printf("Digite o device_used: ");
                fgets(nova.device_used, sizeof(nova.device_used), stdin);
                printf("E fraude? (True/False): ");
                fgets(is_fraud_str, sizeof(is_fraud_str), stdin);

                nova.transaction_id[strcspn(nova.transaction_id, "\n")] = '\0';
                nova.timestamp[strcspn(nova.timestamp, "\n")] = '\0';
                nova.sender_account[strcspn(nova.sender_account, "\n")] = '\0';
                nova.receiver_account[strcspn(nova.receiver_account, "\n")] = '\0';
                nova.transaction_type[strcspn(nova.transaction_type, "\n")] = '\0';
                nova.merchant_category[strcspn(nova.merchant_category, "\n")] = '\0';
                nova.location[strcspn(nova.location, "\n")] = '\0';
                nova.device_used[strcspn(nova.device_used, "\n")] = '\0';
                is_fraud_str[strcspn(is_fraud_str, "\n")] = '\0';
                
                nova.is_fraud = strcmp(is_fraud_str, "True") == 0;
                nova.next = NULL;
                
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

    // Liberar memória da lista
    Transaction* current = head;
    while (current) {
        Transaction* temp = current;
        current = current->next;
        free(temp);
    }

    return 0;
}