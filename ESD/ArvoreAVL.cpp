#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>

// ================= ESTRUTURAS DE DADOS =================

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

typedef struct AVLNode {
    Transaction data;
    struct AVLNode* left;
    struct AVLNode* right;
    uint8_t height;
} AVLNode;

typedef struct Grupo {
    char chave[64];
    int quantidade;
    float soma_valores;
    struct Grupo* prox;
} Grupo;

// ================= FUNÇÕES AVL =================

int max(int a, int b) { return (a > b) ? a : b; }

int height(AVLNode* node) { return node ? node->height : 0; }

int get_balance(AVLNode* node) {
    return node ? height(node->left) - height(node->right) : 0;
}

AVLNode* create_node(Transaction t) {
    AVLNode* node = (AVLNode*)malloc(sizeof(AVLNode));
    if (!node) return NULL;
    node->data = t;
    node->left = node->right = NULL;
    node->height = 1;
    return node;
}

AVLNode* right_rotate(AVLNode* y) {
    AVLNode* x = y->left;
    AVLNode* T2 = x->right;
    x->right = y;
    y->left = T2;
    y->height = max(height(y->left), height(y->right)) + 1;
    x->height = max(height(x->left), height(x->right)) + 1;
    return x;
}

AVLNode* left_rotate(AVLNode* x) {
    AVLNode* y = x->right;
    AVLNode* T2 = y->left;
    y->left = x;
    x->right = T2;
    x->height = max(height(x->left), height(x->right)) + 1;
    y->height = max(height(y->left), height(y->right)) + 1;
    return y;
}
//Inserção de transação
AVLNode* insert_avl(AVLNode* node, Transaction t) {
    if (!node) return create_node(t);
    
    int cmp = strcmp(t.transaction_id, node->data.transaction_id);
    if (cmp < 0)
        node->left = insert_avl(node->left, t);
    else if (cmp > 0)
        node->right = insert_avl(node->right, t);
    else
        return node;

    node->height = 1 + max(height(node->left), height(node->right));
    int balance = get_balance(node);

    if (balance > 1 && cmp < 0)
        return right_rotate(node);
    if (balance < -1 && cmp > 0)
        return left_rotate(node);
    if (balance > 1 && cmp > 0) {
        node->left = left_rotate(node->left);
        return right_rotate(node);
    }
    if (balance < -1 && cmp < 0) {
        node->right = right_rotate(node->right);
        return left_rotate(node);
    }
    return node;
}
//Busca de transação por ID
AVLNode* search_avl(AVLNode* root, const char* id) {
    if (!root) return NULL;
    int cmp = strcmp(id, root->data.transaction_id);
    if (cmp == 0) return root;
    return cmp < 0 ? search_avl(root->left, id) : search_avl(root->right, id);
}

AVLNode* min_value_node(AVLNode* node) {
    while (node->left) node = node->left;
    return node;
}
//Remoção de transação por ID
AVLNode* delete_avl(AVLNode* root, const char* id) {
    if (!root) return NULL;
    
    int cmp = strcmp(id, root->data.transaction_id);
    if (cmp < 0)
        root->left = delete_avl(root->left, id);
    else if (cmp > 0)
        root->right = delete_avl(root->right, id);
    else {
        if (!root->left || !root->right) {
            AVLNode* temp = root->left ? root->left : root->right;
            free(root);
            return temp;
        }
        AVLNode* temp = min_value_node(root->right);
        root->data = temp->data;
        root->right = delete_avl(root->right, temp->data.transaction_id);
    }

    root->height = 1 + max(height(root->left), height(root->right));
    int balance = get_balance(root);

    if (balance > 1 && get_balance(root->left) >= 0)
        return right_rotate(root);
    if (balance > 1 && get_balance(root->left) < 0) {
        root->left = left_rotate(root->left);
        return right_rotate(root);
    }
    if (balance < -1 && get_balance(root->right) <= 0)
        return left_rotate(root);
    if (balance < -1 && get_balance(root->right) > 0) {
        root->right = right_rotate(root->right);
        return left_rotate(root);
    }
    return root;
}

// ================= FUNÇÕES DE ESTATÍSTICAS =================

void coletar_dados(AVLNode* root, float* valores, int* index, int* total_fraudes, 
                  float* soma, float* maior, float* menor) {
    if (!root) return;
    
    coletar_dados(root->left, valores, index, total_fraudes, soma, maior, menor);
    
    valores[*index] = root->data.amount;
    (*index)++;
    *soma += root->data.amount;
    
    if (root->data.amount > *maior) *maior = root->data.amount;
    if (root->data.amount < *menor) *menor = root->data.amount;
    if (root->data.is_fraud) (*total_fraudes)++;
    
    coletar_dados(root->right, valores, index, total_fraudes, soma, maior, menor);
}

int contar_transacoes(AVLNode* root) {
    if (!root) return 0;
    return 1 + contar_transacoes(root->left) + contar_transacoes(root->right);
}

void calcular_estatisticas(AVLNode* root) {
    int total = contar_transacoes(root);
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
    
    coletar_dados(root, valores, &index, &total_fraudes, &soma, &maior, &menor);
    
    // Ordenar para mediana e moda
    qsort(valores, total, sizeof(float), 
         [](const void* a, const void* b) {
             float fa = *(const float*)a, fb = *(const float*)b;
             return (fa > fb) - (fa < fb);
         });

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

// ================= FUNÇÕES DE AGRUPAMENTO =================

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

void agrupar_transacoes(AVLNode* root, Grupo** grupos, int campo) {
    if (!root) return;
    
    agrupar_transacoes(root->left, grupos, campo);
    
    const char* chave = "";
    switch (campo) {
        case 1: chave = root->data.transaction_type; break;
        case 2: chave = root->data.merchant_category; break;
        case 3: chave = root->data.location; break;
        case 4: chave = root->data.device_used; break;
        case 5: chave = root->data.sender_account; break;
        case 6: chave = root->data.receiver_account; break;
        default: chave = "Indefinido";
    }
    
    inserir_grupo(grupos, chave, root->data.amount);
    
    agrupar_transacoes(root->right, grupos, campo);
}

void imprimir_grupos(Grupo* grupos, const char* titulo) {
    printf("\n=== Agrupamento por %s ===\n", titulo);
    printf("%-30s | %-8s | %-12s | %-10s\n", "Campo", "Qtd", "Total", "Media");
    printf("------------------------------------------------------------\n");
    
    while (grupos) {
        printf("%-30s | %-8d | %-12.2f | %-10.2f\n", 
               grupos->chave, 
               grupos->quantidade, 
               grupos->soma_valores, 
               grupos->soma_valores / grupos->quantidade);
        grupos = grupos->prox;
    }
}

void liberar_grupos(Grupo* grupos) {
    while (grupos) {
        Grupo* temp = grupos;
        grupos = grupos->prox;
        free(temp);
    }
}

// ================= FUNÇÕES DE CARREGAMENTO =================

AVLNode* load_csv(const char* filename) {
    AVLNode* root = NULL;
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir o arquivo");
        exit(1);
    }
    
    char line[512];
    fgets(line, sizeof(line), file); // Ignorar cabeçalho
    
    while (fgets(line, sizeof(line), file)) {
        Transaction t;
        char is_fraud_str[6];
        
        int campos_lidos = sscanf(line, "%15[^,],%31[^,],%31[^,],%31[^,],%f,%15[^,],%31[^,],%31[^,],%15[^,],%5[^,\n]",
               t.transaction_id, t.timestamp, t.sender_account, t.receiver_account, &t.amount,
               t.transaction_type, t.merchant_category, t.location, t.device_used, is_fraud_str);
        
        if (campos_lidos == 10) {
            t.is_fraud = strcmp(is_fraud_str, "True") == 0;
            root = insert_avl(root, t);
        } else {
            fprintf(stderr, "Linha ignorada por formato inválido: %s", line);
        }
    }
    
    fclose(file);
    return root;
}

// ================= FUNÇÕES AUXILIARES =================

bool prever_fraude(Transaction* t) {
    return (t->amount > 10000 || strcmp(t->device_used, "unknown") == 0);
}

void limpar_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// ================= MENU PRINCIPAL =================

void exibir_menu() {
    printf("\n=== MENU AVL ===\n");
    printf("1. Buscar transacao\n");
    printf("2. Remover transacao\n");
    printf("3. Inserir nova transacao\n");
    printf("4. Mostrar estatisticas\n");
    printf("5. Agrupar por campo\n");
    printf("6. Filtrar transacoes\n");
    printf("7. Sair\n");
    printf("Escolha uma opcao: ");
}

void exibir_campos_agrupamento() {
    printf("\nAgrupar por:\n");
    printf("1. Tipo de Transacao\n");
    printf("2. Categoria do Comerciante\n");
    printf("3. Localizacao\n");
    printf("4. Dispositivo Usado\n");
    printf("5. Conta do Remetente\n");
    printf("6. Conta do Destinatario\n");
    printf("Escolha o campo: ");
}

void exibir_campos_filtragem() {
    printf("\nFiltrar por:\n");
    printf("1. Valor Minimo da Transacao\n");
    printf("2. Tipo de Transacao\n");
    printf("3. Categoria do Comerciante\n");
    printf("4. Localizacao\n");
    printf("5. Dispositivo Usado\n");
    printf("6. Conta do Remetente\n");
    printf("7. Conta do Destinatario\n");
    printf("Escolha o campo: ");
}

void exibir_transacao(Transaction* t) {
    printf("\nTransacao encontrada:\n");
    printf("ID: %s\n", t->transaction_id);
    printf("Timestamp: %s\n", t->timestamp);
    printf("De: %s | Para: %s\n", t->sender_account, t->receiver_account);
    printf("Valor: %.2f | Fraude: %s\n", t->amount, t->is_fraud ? "Sim" : "Nao");
    printf("Tipo: %s | Categoria: %s\n", t->transaction_type, t->merchant_category);
    printf("Local: %s | Dispositivo: %s\n", t->location, t->device_used);
    printf("Previsao de Fraude: %s\n", prever_fraude(t) ? "Sim" : "Nao");
}

void filtrar_transacoes_avl(AVLNode* root, int campo, const char* criterio_str, float criterio_valor, bool is_numerico) {
    if (!root) return;

    filtrar_transacoes_avl(root->left, campo, criterio_str, criterio_valor, is_numerico);

    bool exibir = false;
    if (is_numerico) {
        if (campo == 1 && root->data.amount >= criterio_valor)
            exibir = true;
    } else {
        const char* valor = "";
        switch (campo) {
            case 2: valor = root->data.transaction_type; break;
            case 3: valor = root->data.merchant_category; break;
            case 4: valor = root->data.location; break;
            case 5: valor = root->data.device_used; break;
            case 6: valor = root->data.sender_account; break;
            case 7: valor = root->data.receiver_account; break;
            default: valor = ""; break;
        }
        if (strcmp(valor, criterio_str) == 0)
            exibir = true;
    }

    if (exibir) {
        exibir_transacao(&(root->data));
    }

    filtrar_transacoes_avl(root->right, campo, criterio_str, criterio_valor, is_numerico);
}

// ================= PROGRAMA PRINCIPAL =================

int main() {
    AVLNode* root = load_csv("C:\\Users\\leozi\\OneDrive\\Desktop\\TrabalhoESD.csv\\financial_fraud_detection_dataset.csv");
    int opcao;
    char id[16];

    do {
        exibir_menu();
        scanf("%d", &opcao);
        limpar_buffer();

        switch (opcao) {
            case 1: {
                printf("Digite o ID da transacao: ");
                fgets(id, sizeof(id), stdin);
                id[strcspn(id, "\n")] = '\0';
                
                AVLNode* node = search_avl(root, id);
                if (node) {
                    exibir_transacao(&node->data);
                } else {
                    printf("Transacao nao encontrada.\n");
                }
                break;
            }
                
            case 2: {
                printf("Digite o ID da transacao para remover: ");
                fgets(id, sizeof(id), stdin);
                id[strcspn(id, "\n")] = '\0';
                
                root = delete_avl(root, id);
                printf("Transacao removida com sucesso.\n");
                break;
            }
                
            case 3: {
                Transaction t;
                char is_fraud_str[6];
                
                printf("Digite o ID da transacao: ");
                fgets(t.transaction_id, sizeof(t.transaction_id), stdin);
                printf("Digite o timestamp: ");
                fgets(t.timestamp, sizeof(t.timestamp), stdin);
                printf("Digite a conta do remetente: ");
                fgets(t.sender_account, sizeof(t.sender_account), stdin);
                printf("Digite a conta do destinatario: ");
                fgets(t.receiver_account, sizeof(t.receiver_account), stdin);
                printf("Digite o valor: ");
                scanf("%f", &t.amount);
                limpar_buffer();
                printf("Digite o tipo de transacao: ");
                fgets(t.transaction_type, sizeof(t.transaction_type), stdin);
                printf("Digite a categoria do comerciante: ");
                fgets(t.merchant_category, sizeof(t.merchant_category), stdin);
                printf("Digite a localizacao: ");
                fgets(t.location, sizeof(t.location), stdin);
                printf("Digite o dispositivo usado: ");
                fgets(t.device_used, sizeof(t.device_used), stdin);
                printf("E fraude? (True/False): ");
                fgets(is_fraud_str, sizeof(is_fraud_str), stdin);
                
                // Remover quebras de linha
                t.transaction_id[strcspn(t.transaction_id, "\n")] = '\0';
                t.timestamp[strcspn(t.timestamp, "\n")] = '\0';
                t.sender_account[strcspn(t.sender_account, "\n")] = '\0';
                t.receiver_account[strcspn(t.receiver_account, "\n")] = '\0';
                t.transaction_type[strcspn(t.transaction_type, "\n")] = '\0';
                t.merchant_category[strcspn(t.merchant_category, "\n")] = '\0';
                t.location[strcspn(t.location, "\n")] = '\0';
                t.device_used[strcspn(t.device_used, "\n")] = '\0';
                is_fraud_str[strcspn(is_fraud_str, "\n")] = '\0';
                
                t.is_fraud = strcmp(is_fraud_str, "True") == 0;
                root = insert_avl(root, t);
                printf("Transacao inserida com sucesso!\n");
                break;
            }
                
            case 4:
                calcular_estatisticas(root);
                break;
                
            case 5: {
                exibir_campos_agrupamento();
                int campo;
                scanf("%d", &campo);
                limpar_buffer();
                
                if (campo < 1 || campo > 6) {
                    printf("Opcao invalida!\n");
                    break;
                }
                
                Grupo* grupos = NULL;
                agrupar_transacoes(root, &grupos, campo);
                
                const char* titulos[] = {"", "Tipo de Transacao", "Categoria do Comerciante", 
                                        "Localizacao", "Dispositivo Usado", 
                                        "Conta do Remetente", "Conta do Destinatario"};
                imprimir_grupos(grupos, titulos[campo]);
                liberar_grupos(grupos);
                break;
            }
            case 6: {
              exibir_campos_filtragem();
              int campo;
              scanf("%d", &campo);
              limpar_buffer();
              //Filtragem e ordenação dos dados
              if (campo == 1) {
              float min_valor;
              printf("Digite o valor minimo da transacao: ");
              scanf("%f", &min_valor);
              limpar_buffer();
              filtrar_transacoes_avl(root, campo, "", min_valor, true);
              } else if (campo >= 2 && campo <= 7) {
              char criterio[64];
              printf("Digite o valor do campo para filtrar: ");
              fgets(criterio, sizeof(criterio), stdin);
              criterio[strcspn(criterio, "\n")] = '\0'; 
              filtrar_transacoes_avl(root, campo, criterio, 0, false);
              } else {
              printf("Campo invalido.\n");
              }
              break;
           }
   
            case 7:
                printf("Encerrando o programa...\n");
                break;
                
            default:
                printf("Opcao invalida! Tente novamente.\n");
        }
    } while (opcao != 7);

    // Liberar memória da árvore
    // (Implementação de liberação de memória da árvore seria necessária aqui)
    
    return 0;
}