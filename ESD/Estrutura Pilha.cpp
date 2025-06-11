#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

#define MAX_STACK_SIZE 5000000

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
} Transaction;

typedef struct Grupo {
    char chave[64];
    int quantidade;
    float soma_valores;
    struct Grupo* prox;
} Grupo;

Transaction stack[MAX_STACK_SIZE];
int top = -1;

// Funções básicas da pilha
bool is_full() {
    return top == MAX_STACK_SIZE - 1;
}

bool is_empty() {
    return top == -1;
}
//Inserção de transação na cabeça
void push(Transaction t) {
    if (is_full()) {
        printf("Erro: Pilha cheia.\n");
        return;
    }
    stack[++top] = t;
}
//Remoção de transação da cabeça
void pop() {
    if (is_empty()) {
        printf("Erro: Pilha vazia.\n");
        return;
    }
    printf("Transação %s removida do topo.\n", stack[top].transaction_id);
    top--;
}
//Busca na pilha por ID
Transaction* search_transaction(const char* transaction_id) {
    for (int i = top; i >= 0; i--) {
        if (strcmp(stack[i].transaction_id, transaction_id) == 0) {
            return &stack[i];
        }
    }
    return NULL;
}

// Funções para estatísticas
void coletar_dados(float* valores, int* index, int* total_fraudes, 
                  float* soma, float* maior, float* menor) {
    for (int i = 0; i <= top; i++) {
        valores[*index] = stack[i].amount;
        (*index)++;
        *soma += stack[i].amount;
        
        if (stack[i].amount > *maior) *maior = stack[i].amount;
        if (stack[i].amount < *menor) *menor = stack[i].amount;
        if (stack[i].is_fraud) (*total_fraudes)++;
    }
}

int compare_floats(const void* a, const void* b) {
    float fa = *(const float*)a;
    float fb = *(const float*)b;
    return (fa > fb) - (fa < fb);
}

void calcular_estatisticas() {
    int total = top + 1;
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
    
    qsort(valores, total, sizeof(float), compare_floats);

    float mediana = (total % 2 == 0) ? 
                   (valores[total/2 - 1] + valores[total/2]) / 2 : 
                   valores[total/2];

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
    
    for (int i = 0; i <= top; i++) {
        const char* chave = "";
        switch (campo) {
            case 1: chave = stack[i].transaction_type; titulo = "Tipo de Transacao"; break;
            case 2: chave = stack[i].merchant_category; titulo = "Categoria do Comerciante"; break;
            case 3: chave = stack[i].location; titulo = "Localizacao"; break;
            case 4: chave = stack[i].device_used; titulo = "Dispositivo Usado"; break;
            case 5: chave = stack[i].sender_account; titulo = "Conta do Remetente"; break;
            case 6: chave = stack[i].receiver_account; titulo = "Conta do Destinatario"; break;
            default: chave = "Indefinido"; titulo = "Indefinido";
        }
        
        inserir_grupo(&grupos, chave, stack[i].amount);
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

// Função para previsão de fraude
bool prever_fraude(Transaction* t) {
    return (t->amount > 10000 || strcmp(t->device_used, "unknown") == 0);
}

// Carregar Dataset 
void load_csv(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir o arquivo");
        exit(1);
    }

    char line[512];
    fgets(line, sizeof(line), file);

    while (fgets(line, sizeof(line), file)) {
        Transaction t;
        char is_fraud_str[6];

        int campos = sscanf(line, "%15[^,],%31[^,],%31[^,],%31[^,],%f,%15[^,],%31[^,],%31[^,],%15[^,],%5[^,\n]",
                            t.transaction_id, t.timestamp, t.sender_account, t.receiver_account,
                            &t.amount, t.transaction_type, t.merchant_category,
                            t.location, t.device_used, is_fraud_str);

        if (campos == 10) {
            t.is_fraud = strcmp(is_fraud_str, "True") == 0;
            push(t);
        } else {
            fprintf(stderr, "Linha ignorada (inválida): %s", line);
        }
    }

    fclose(file);
}
//Filtragem dos dados 
void filtrar_transacoes(float valor_min, float valor_max, const char* tipo) {
    printf("\n=== Transacoes Filtradas ===\n");
    printf("%-16s | %-10s | %-10s | %-10s | %-8s | %-12s\n",
           "ID", "Remetente", "Destinatario", "Tipo", "Valor", "Fraude");

    printf("--------------------------------------------------------------------------\n");

    for (int i = top; i >= 0; i--) {
        Transaction* t = &stack[i];
        if ((valor_min < 0 || t->amount >= valor_min) &&
            (valor_max < 0 || t->amount <= valor_max) &&
            (strlen(tipo) == 0 || strcmp(t->transaction_type, tipo) == 0)) {
            printf("%-16s | %-10s | %-10s | %-10s | %-8.2f | %-6s\n",
                   t->transaction_id, t->sender_account, t->receiver_account,
                   t->transaction_type, t->amount, t->is_fraud ? "Sim" : "Nao");
        }
    }
}

int compare_transacoes(const void* a, const void* b) {
    const Transaction* t1 = (const Transaction*)a;
    const Transaction* t2 = (const Transaction*)b;
    return (t1->amount > t2->amount) - (t1->amount < t2->amount);  // crescente
}

int compare_transacoes_decrescente(const void* a, const void* b) {
    return compare_transacoes(b, a);  // apenas invertido
}
//Ordenação dos dados
void ordenar_transacoes(bool crescente) {
    if (is_empty()) {
        printf("Nenhuma transacao para ordenar.\n");
        return;
    }

    Transaction* copia = (Transaction*)malloc((top + 1) * sizeof(Transaction));
    if (!copia) {
        printf("Erro de memoria.\n");
        return;
    }

    memcpy(copia, stack, (top + 1) * sizeof(Transaction));

    qsort(copia, top + 1, sizeof(Transaction),
          crescente ? compare_transacoes : compare_transacoes_decrescente);

    printf("\n=== Transacoes Ordenadas (%s) ===\n", crescente ? "Crescente" : "Decrescente");
    printf("%-16s | %-10s | %-10s | %-10s | %-8s | %-6s\n",
           "ID", "Remetente", "Destinatario", "Tipo", "Valor", "Fraude");
    printf("-------------------------------------------------------------------------\n");

    for (int i = 0; i <= top; i++) {
        Transaction* t = &copia[i];
        printf("%-16s | %-10s | %-10s | %-10s | %-8.2f | %-6s\n",
               t->transaction_id, t->sender_account, t->receiver_account,
               t->transaction_type, t->amount, t->is_fraud ? "Sim" : "Nao");
    }

    free(copia);
}

// Menu principal
int main() {
    load_csv("C:\\Users\\leozi\\OneDrive\\Desktop\\TrabalhoESD.csv\\financial_fraud_detection_dataset.csv");

    int opcao;
    char id[16];

    do {
        printf("\n=== MENU (PILHA) ===\n");
        printf("1. Buscar transacao\n");
        printf("2. Remover transacao (pop)\n");
        printf("3. Inserir nova transacao (push)\n");
        printf("4. Sair\n");
        printf("5. Mostrar estatisticas\n");
        printf("6. Agrupar por campo\n");
        printf("7. Filtrar transacoes\n");
        printf("8. Ordenar transacoes\n");
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
                }
                break;

            case 2:
                pop();
                break;

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
                
                push(nova);
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
            case 7: {
              float vmin, vmax;
              char tipo[16];

              printf("Valor minimo (-1 para ignorar): ");
              scanf("%f", &vmin);
              getchar();
              printf("Valor maximo (-1 para ignorar): ");
              scanf("%f", &vmax);
              getchar();
              printf("Tipo de transacao (vazio para ignorar): ");
              fgets(tipo, sizeof(tipo), stdin);
              tipo[strcspn(tipo, "\n")] = '\0';
              filtrar_transacoes(vmin, vmax, tipo);
              break;
            }

            case 8: {
              int ordem;
              printf("Ordenar por valor:\n1. Crescente\n2. Decrescente\nEscolha: ");
              scanf("%d", &ordem);
              getchar();

              ordenar_transacoes(ordem == 1);
              break;
            }


            default:
                printf("Opcao invalida.\n");
        }

    } while (opcao != 4);

    return 0;
}