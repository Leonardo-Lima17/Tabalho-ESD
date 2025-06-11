#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <stdint.h>
#include <windows.h> // Necessario para HighPrecisionTimer e Sleep
#include <time.h>    // Necessario para srand, time

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#define NUM_REPETITIONS 10 // Numero de repeticoes para cada benchmark individual

// ================= ESTRUTURAS DE DADOS =================

// Estrutura para representar uma transacao
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

// Estrutura para um no de arvore AVL
typedef struct AVLNode {
    int key; // Chave numerica usada para a ordenacao da arvore AVL
    Transaction data; // Dados da transacao armazenados no no
    struct AVLNode* left; // Ponteiro para o filho esquerdo
    struct AVLNode* right; // Ponteiro para o filho direito
    uint8_t height; // Altura do no na arvore
} AVLNode;

// Estrutura para a Arvore AVL, contendo a raiz e o numero de elementos
typedef struct AVLTree {
    AVLNode* root; // Ponteiro para a raiz da arvore
    int size;      // Numero de elementos na arvore
} AVLTree;

// Estrutura para simular MachineData para o printf
typedef struct MachineData_Simulated {
    int UDI;
    char ProductID[10];
    char Type;
    float AirTemp;
    float ProcessTemp;
    int RotationalSpeed;
    float Torque;
    int ToolWear;
    bool MachineFailure;
    bool TWF;
    bool HDF;
    bool PWF;
    bool OSF;
    bool RNF;
} MachineData_Simulated;

// ================= FUNCOES AUXILIARES AVL =================

// Retorna o maximo entre dois inteiros
int max(int a, int b) { return (a > b) ? a : b; }

// Retorna a altura de um no (0 se o no for NULL)
int height(AVLNode* node) { return node ? node->height : 0; }

// Retorna o fator de balanceamento de um no (altura do filho esquerdo - altura do filho direito)
int get_balance(AVLNode* node) {
    return node ? height(node->left) - height(node->right) : 0;
}

// Cria um novo no com a chave e os dados fornecidos
AVLNode* create_node(int key, Transaction data) {
    AVLNode* node = (AVLNode*)malloc(sizeof(AVLNode));
    if (node == NULL) {
        perror("Erro de alocacao de memoria para AVLNode");
        exit(EXIT_FAILURE);
    }
    node->key = key;
    node->data = data;
    node->left = NULL;
    node->right = NULL;
    node->height = 1; // Novo no tem altura 1
    return node;
}

// Rotacao a direita no no y
AVLNode* right_rotate(AVLNode* y) {
    AVLNode* x = y->left;
    AVLNode* T2 = x->right;

    x->right = y;
    y->left = T2;

    // Atualiza alturas
    y->height = 1 + max(height(y->left), height(y->right));
    x->height = 1 + max(height(x->left), height(x->right));

    return x; // Retorna a nova raiz da subarvore rotacionada
}

// Rotacao a esquerda no no x
AVLNode* left_rotate(AVLNode* x) {
    AVLNode* y = x->right;
    AVLNode* T2 = y->left;

    y->left = x;
    x->right = T2;

    // Atualiza alturas
    x->height = 1 + max(height(x->left), height(x->right));
    y->height = 1 + max(height(y->left), height(y->right));

    return y; // Retorna a nova raiz da subarvore rotacionada
}

// ================= FUNCOES AVL (ADAPTADAS PARA BENCHMARKS) =================

// Funcao recursiva de insercao AVL (controla o tamanho)
AVLNode* insert_node_avl_benchmark(AVLNode* node, int key, Transaction data, int* size_ptr) {
    // 1. Realiza a insercao BST padrao
    if (node == NULL) {
        if (size_ptr) (*size_ptr)++; // Incrementa o tamanho da arvore
        return create_node(key, data);
    }

    if (key < node->key) {
        node->left = insert_node_avl_benchmark(node->left, key, data, size_ptr);
    } else if (key > node->key) {
        node->right = insert_node_avl_benchmark(node->right, key, data, size_ptr);
    } else { // Chave duplicada
        return node; 
    }

    // 2. Atualiza a altura deste no ancestral
    node->height = 1 + max(height(node->left), height(node->right));

    // 3. Obtem o fator de balanceamento deste no ancestral
    int balance = get_balance(node);

    // 4. Se este no se torna desbalanceado, ha 4 casos:
    // Caso Esquerda-Esquerda
    if (balance > 1 && key < node->left->key)
        return right_rotate(node);

    // Caso Direita-Direita
    if (balance < -1 && key > node->right->key)
        return left_rotate(node);

    // Caso Esquerda-Direita
    if (balance > 1 && key > node->left->key) {
        node->left = left_rotate(node->left);
        return right_rotate(node);
    }

    // Caso Direita-Esquerda
    if (balance < -1 && key < node->right->key) {
        node->right = right_rotate(node->right);
        return left_rotate(node);
    }

    return node;
}

// Wrapper para insercao AVL
void insertAVLTree(AVLTree* tree, int key, Transaction data) {
    tree->root = insert_node_avl_benchmark(tree->root, key, data, &(tree->size));
}

// Funcao recursiva de busca AVL
AVLNode* search_avl_node_benchmark(AVLNode* node, int key) {
    if (node == NULL || node->key == key) {
        return node;
    }
    if (node->key < key) {
        return search_avl_node_benchmark(node->right, key);
    }
    return search_avl_node_benchmark(node->left, key);
}

// Wrapper para busca AVL
AVLNode* searchAVLTree(AVLTree* tree, int key) {
    return search_avl_node_benchmark(tree->root, key);
}

// Funcao recursiva de delecao AVL (controla o tamanho)
AVLNode* delete_node_avl_benchmark(AVLNode* root, int key, int* size_ptr) {
    // PASSO 1: Realiza a delecao BST padrao
    if (root == NULL) return root;

    if (key < root->key) {
        root->left = delete_node_avl_benchmark(root->left, key, size_ptr);
    } else if (key > root->key) {
        root->right = delete_node_avl_benchmark(root->right, key, size_ptr);
    } else { // No com a chave a ser deletada encontrado
        // No com apenas um filho ou sem filho
        if ((root->left == NULL) || (root->right == NULL)) {
            AVLNode* temp = root->left ? root->left : root->right;

            // Caso sem filho
            if (temp == NULL) {
                temp = root;
                root = NULL;
            } else { // Caso com um filho
                *root = *temp; 
            }
            free(temp);
            if (size_ptr) (*size_ptr)--; 
        } else {
            // No com dois filhos: Obtem o sucessor in-order (menor na subarvore direita)
            AVLNode* temp = root->right;
            while (temp->left != NULL) {
                temp = temp->left;
            }

            // Copia a chave e os dados do sucessor in-order para este no
            root->key = temp->key;
            root->data = temp->data;

            // Deleta o sucessor in-order
            root->right = delete_node_avl_benchmark(root->right, temp->key, size_ptr);
        }
    }

    // Se a arvore tinha apenas um no, retorna
    if (root == NULL) return root;

    // PASSO 2: Atualiza a altura do no atual
    root->height = 1 + max(height(root->left), height(root->right));

    // PASSO 3: Obtem o fator de balanceamento deste no
    int balance = get_balance(root);

    // Se este no se torna desbalanceado, ha 4 casos:
    // Caso Esquerda-Esquerda
    if (balance > 1 && get_balance(root->left) >= 0)
        return right_rotate(root);

    // Caso Esquerda-Direita
    if (balance > 1 && get_balance(root->left) < 0) {
        root->left = left_rotate(root->left);
        return right_rotate(root);
    }

    // Caso Direita-Direita
    if (balance < -1 && get_balance(root->right) <= 0)
        return left_rotate(root);

    // Caso Direita-Esquerda
    if (balance < -1 && get_balance(root->right) > 0) {
        root->right = right_rotate(root->right);
        return left_rotate(root);
    }

    return root;
}

// Wrapper para delecao AVL
void deleteAVLTree(AVLTree* tree, int key) {
    tree->root = delete_node_avl_benchmark(tree->root, key, &(tree->size));
}

// Libera recursivamente todos os nos na arvore
void freeAVLTree(AVLNode* node) {
    if (node == NULL) return;
    freeAVLTree(node->left);
    freeAVLTree(node->right);
    free(node);
}

// Inicializa uma estrutura AVLTree
void initAVLTree(AVLTree* tree) {
    tree->root = NULL;
    tree->size = 0;
}

// ================= FUNCOES AUXILIARES PARA ESTATISTICAS =================

// Calcula a media de um array de doubles
double calculate_mean(double* data, int count) {
    double sum = 0.0;
    for (int i = 0; i < count; i++) {
        sum += data[i];
    }
    return sum / count;
}

// Calcula o desvio padrao de um array de doubles
double calculate_std_dev(double* data, int count, double mean) {
    double sum_sq_diff = 0.0;
    for (int i = 0; i < count; i++) {
        sum_sq_diff += pow(data[i] - mean, 2);
    }
    return sqrt(sum_sq_diff / count);
}

// Calcula o coeficiente de variacao
double calculate_coeff_of_variation(double mean, double std_dev) {
    if (mean == 0) return 0.0; // Evita divisao por zero
    return (std_dev / mean) * 100.0; // Em porcentagem
}

// ================= FUNCOES DE BENCHMARK =================

// Estrutura para timer de alta precisao (Windows specific)
typedef struct {
    LARGE_INTEGER start;
    LARGE_INTEGER end;
    LARGE_INTEGER frequency;
} HighPrecisionTimer;

// Inicia o timer
void start_timer(HighPrecisionTimer* timer) {
    QueryPerformanceFrequency(&timer->frequency);
    QueryPerformanceCounter(&timer->start);
}

// Para o timer e retorna o tempo decorrido em milissegundos
double stop_timer(HighPrecisionTimer* timer) {
    QueryPerformanceCounter(&timer->end);
    double elapsed = (double)(timer->end.QuadPart - timer->start.QuadPart) * 1000.0 / timer->frequency.QuadPart;
    return elapsed;
}

// Gera um conjunto de dados aleatorios e os insere em uma AVLTree
void generateRandomData(AVLTree* tree, int count) {
    for (int i = 0; i < count; i++) {
        Transaction d = {0}; 
        int udi_key = 10000 + i + rand() % 100000; // Garante UDI unico para AVL

        // Preenche os campos da transacao com dados aleatorios
        snprintf(d.transaction_id, sizeof(d.transaction_id), "TRN%08d", udi_key);
        snprintf(d.timestamp, sizeof(d.timestamp), "2024-01-01 %02d:%02d:%02d", rand() % 24, rand() % 60, rand() % 60);
        snprintf(d.sender_account, sizeof(d.sender_account), "ACC%05d", rand() % 100000);
        snprintf(d.receiver_account, sizeof(d.receiver_account), "ACC%05d", rand() % 100000);
        d.amount = 10.0f + (rand() % 10000) / 100.0f;
        char types[][16] = {"purchase", "transfer", "deposit", "withdrawal", "payment"};
        snprintf(d.transaction_type, sizeof(d.transaction_type), "%s", types[rand() % 5]);
        char categories[][32] = {"retail", "food", "travel", "online", "services", "utilities", "healthcare"};
        snprintf(d.merchant_category, sizeof(d.merchant_category), "%s", categories[rand() % 7]);
        snprintf(d.location, sizeof(d.location), "City%03d", rand() % 500);
        char devices[][16] = {"mobile", "web", "pos", "atm"};
        snprintf(d.device_used, sizeof(d.device_used), "%s", devices[rand() % 4]);
        d.is_fraud = (rand() % 100 < 5); // 5% de chance de ser fraude

        insertAVLTree(tree, udi_key, d);
    }
}

// Realiza o benchmark de insercao
void benchmark_insertion(AVLTree* tree_param, int num_elements) {
    double results[NUM_REPETITIONS];
    printf("\nBenchmark Insercao (%d elementos):\n", num_elements);

    for (int i = 0; i < NUM_REPETITIONS; i++) {
        HighPrecisionTimer t;
        AVLTree tmp; 
        initAVLTree(&tmp); // Sempre inicia com uma arvore limpa
        
        start_timer(&t);
        generateRandomData(&tmp, num_elements);
        results[i] = stop_timer(&t);
        
        freeAVLTree(tmp.root); // Libera memoria da arvore temporaria
    }

    double mean = calculate_mean(results, NUM_REPETITIONS);
    double std_dev = calculate_std_dev(results, NUM_REPETITIONS, mean);
    double cov = calculate_coeff_of_variation(mean, std_dev);

    printf("  Media: %.3f ms\n", mean);
    printf("  Desvio Padrao: %.3f ms\n", std_dev);
    printf("  Coeficiente de Variacao: %.2f%%\n", cov);
}

// Realiza o benchmark de busca
void benchmark_search(AVLTree* tree) {
    if (tree->size == 0) {
        printf("Arvore vazia para busca\n"); 
        return;
    }

    double results[NUM_REPETITIONS];
    printf("\nBenchmark Busca (%d ops):\n", 10000); // 10000 ops e o numero de buscas por repeticao

    // Encontra o menor e maior UDI para o intervalo de busca (fora do loop de repeticoes para consistencia)
    AVLNode* minNode = tree->root;
    while (minNode != NULL && minNode->left != NULL) {
        minNode = minNode->left;
    }
    AVLNode* maxNode = tree->root;
    while (maxNode != NULL && maxNode->right != NULL) {
        maxNode = maxNode->right;
    }
    
    int minUDI = minNode ? minNode->key : 0; 
    int maxUDI = maxNode ? maxNode->key : 0; 

    // Ajusta o intervalo se a arvore for muito pequena
    if (maxUDI < minUDI) { 
        if (minNode) {
            maxUDI = minUDI + 1;
        } else {
            minUDI = 0; maxUDI = 1000000; 
        }
    }

    for (int i = 0; i < NUM_REPETITIONS; i++) {
        HighPrecisionTimer t;
        const int searches_per_run = 10000;
        long long found = 0; 

        start_timer(&t);
        for (int j = 0; j < searches_per_run; j++) {
            int search_udi = minUDI + (rand() % (maxUDI - minUDI + 1)); 
            if (searchAVLTree(tree, search_udi) != NULL) {
                found++;
            }
        }
        results[i] = stop_timer(&t);
    }

    double mean = calculate_mean(results, NUM_REPETITIONS);
    double std_dev = calculate_std_dev(results, NUM_REPETITIONS, mean);
    double cov = calculate_coeff_of_variation(mean, std_dev);

    printf("  Media: %.3f ms\n", mean);
    printf("  Desvio Padrao: %.3f ms\n", std_dev);
    printf("  Coeficiente de Variacao: %.2f%%\n", cov);
}

// Realiza o benchmark de remocao
void benchmark_removal(AVLTree* tree) {
    if (tree->size == 0) {
        printf("Arvore vazia para remocao\n"); 
        return;
    }

    double results[NUM_REPETITIONS];
    printf("\nBenchmark Remocao (%d ops):\n", 1000); // 1000 ops e o numero de remocoes por repeticao

    for (int i = 0; i < NUM_REPETITIONS; i++) {
        HighPrecisionTimer t;
        AVLTree tmp; 
        initAVLTree(&tmp);
        
        // Copia os elementos para uma arvore temporaria para remocao
        AVLNode** stack = (AVLNode**)malloc((tree->size + 1) * sizeof(AVLNode*)); 
        if (!stack) {
            perror("Erro ao alocar stack para travessia");
            return;
        }
        int top = -1;
        AVLNode* current_original = tree->root;
        while (current_original != NULL || top != -1) {
            while (current_original != NULL) {
                stack[++top] = current_original;
                current_original = current_original->left;
            }
            current_original = stack[top--];
            insertAVLTree(&tmp, current_original->key, current_original->data);
            current_original = current_original->right;
        }
        free(stack);

        const int removals_per_run = (tmp.size < 1000) ? tmp.size / 2 : 1000; 
        if (removals_per_run == 0 && tmp.size > 0) {
            // Nao ha remocoes suficientes para 1000 ops, ajusta.
        }

        // Coleta as chaves existentes na arvore temporaria para remocao
        int* keys_to_remove = (int*)malloc(tmp.size * sizeof(int));
        if (keys_to_remove == NULL) {
            perror("Erro ao alocar memoria para keys_to_remove");
            freeAVLTree(tmp.root);
            return;
        }
        int count_keys = 0;
        AVLNode** temp_stack = (AVLNode**)malloc((tmp.size + 1) * sizeof(AVLNode*)); 
        if (!temp_stack) {
            perror("Erro ao alocar temp_stack para travessia");
            free(keys_to_remove);
            freeAVLTree(tmp.root);
            return;
        }
        top = -1;
        AVLNode* current_tmp = tmp.root;
        while (current_tmp != NULL || top != -1) {
            while (current_tmp != NULL) {
                temp_stack[++top] = current_tmp;
                current_tmp = current_tmp->left;
            }
            current_tmp = temp_stack[top--];
            if (count_keys < tmp.size) { 
                keys_to_remove[count_keys++] = current_tmp->key;
            }
            current_tmp = current_tmp->right;
        }
        free(temp_stack);

        // Embaralha as chaves para remocao aleatoria
        for (int k = count_keys - 1; k > 0; k--) {
            int l = rand() % (k + 1);
            int temp_key = keys_to_remove[k];
            keys_to_remove[k] = keys_to_remove[l];
            keys_to_remove[l] = temp_key;
        }

        start_timer(&t);
        for (int j = 0; j < removals_per_run; j++) {
            if (tmp.size > 0 && j < count_keys) { 
                deleteAVLTree(&tmp, keys_to_remove[j]); 
            }
        }
        results[i] = stop_timer(&t);

        free(keys_to_remove);
        freeAVLTree(tmp.root); 
    }

    double mean = calculate_mean(results, NUM_REPETITIONS);
    double std_dev = calculate_std_dev(results, NUM_REPETITIONS, mean);
    double cov = calculate_coeff_of_variation(mean, std_dev);

    printf("  Media: %.3f ms\n", mean);
    printf("  Desvio Padrao: %.3f ms\n", std_dev);
    printf("  Coeficiente de Variacao: %.2f%%\n", cov);
}

// Calcula o tamanho em bytes de um no AVL
size_t calculate_node_size() {
    return sizeof(AVLNode); 
}

// Estima o uso de memoria da arvore
void estimate_memory_usage(AVLTree* tree) {
    if (tree == NULL) {
        printf("Arvore invalida\n"); 
        return;
    }

    size_t node_size = calculate_node_size();
    size_t total_memory = tree->size * node_size + sizeof(AVLTree);
    
    printf("\n=== ESTIMATIVA DE USO DE MEMORIA ===\n"); 
    printf("Tamanho por no: %zu bytes\n", node_size); 
    printf("Numero de nos: %d\n", tree->size); 
    printf("Memoria total estimada: %zu bytes (%.2f KB)\n", 
           total_memory, (float)total_memory / 1024);

    printf("\nComparacao com sizeof:\n"); 
    printf("sizeof(MachineData): %zu bytes\n", sizeof(Transaction)); 
    printf("sizeof(AVLNode): %zu bytes\n", sizeof(AVLNode));
    printf("Obs: Pode haver padding/alignment pelo compilador\n"); 
}

// Realiza o benchmark de acesso aleatorio
void benchmark_random_access(AVLTree* tree) {
    if (tree->size == 0) {
        printf("Arvore vazia para teste de acesso aleatorio\n"); 
        return;
    }

    double results[NUM_REPETITIONS];
    printf("\nBenchmark Acesso Aleatorio (%d acessos):\n", 10000); // 10000 acessos por repeticao

    // Encontra o menor e maior UDI para o intervalo de acesso (fora do loop de repeticoes)
    AVLNode* minNode = tree->root;
    while (minNode != NULL && minNode->left != NULL) {
        minNode = minNode->left;
    }
    AVLNode* maxNode = tree->root;
    while (maxNode != NULL && maxNode->right != NULL) {
        maxNode = maxNode->right;
    }
    
    int minUDI = minNode ? minNode->key : 0; 
    int maxUDI = maxNode ? maxNode->key : 0; 

    if (maxUDI < minUDI) { 
         if (minNode) {
            maxUDI = minUDI + 1;
        } else {
            minUDI = 0; maxUDI = 1000000;
        }
    }

    for (int i = 0; i < NUM_REPETITIONS; i++) {
        HighPrecisionTimer t;
        const int accesses_per_run = 10000;
        long long sum = 0; 

        start_timer(&t);
        for (int j = 0; j < accesses_per_run; j++) {
            int random_udi = minUDI + (rand() % (maxUDI - minUDI + 1)); 
            AVLNode* found = searchAVLTree(tree, random_udi);
            if (found != NULL) {
                sum += found->key; 
            }
        }
        results[i] = stop_timer(&t);
    }

    double mean = calculate_mean(results, NUM_REPETITIONS);
    double std_dev = calculate_std_dev(results, NUM_REPETITIONS, mean);
    double cov = calculate_coeff_of_variation(mean, std_dev);

    printf("  Media: %.3f ms\n", mean);
    printf("  Desvio Padrao: %.3f ms\n", std_dev);
    printf("  Coeficiente de Variacao: %.2f%%\n", cov);
}

// Benchmark de escalabilidade
void benchmark_scalability() {
    printf("\nBenchmark de Escalabilidade:\n"); 
    int sizes[] = {1000, 5000, 10000, 20000, 50000, 100000}; 
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < num_sizes; s++) { // Loop pelos tamanhos
        int current_size = sizes[s];
        double results[NUM_REPETITIONS];

        for (int i = 0; i < NUM_REPETITIONS; i++) { // Repeticoes para cada tamanho
            AVLTree tree;
            initAVLTree(&tree);

            HighPrecisionTimer t;
            start_timer(&t);
            generateRandomData(&tree, current_size);
            results[i] = stop_timer(&t);
            freeAVLTree(tree.root);
        }
        double mean = calculate_mean(results, NUM_REPETITIONS);
        double std_dev = calculate_std_dev(results, NUM_REPETITIONS, mean);
        double cov = calculate_coeff_of_variation(mean, std_dev);

        printf("  Tamanho: %6d elementos | Media: %7.3f ms | DP: %7.3f ms | CV: %.2f%%\n",
               current_size, mean, std_dev, cov);
    }
}

// Benchmark de operacoes combinadas
void benchmark_combined_operations(AVLTree* tree) {
    double results[NUM_REPETITIONS];
    printf("\nBenchmark Operacoes Combinadas (%d ops):\n", 1000); // 1000 ops por repeticao

    for (int i = 0; i < NUM_REPETITIONS; i++) {
        HighPrecisionTimer t;
        const int num_operations_per_run = 1000;
        
        // Crie uma copia da arvore para cada repeticao para garantir o estado inicial
        AVLTree tmp_tree_for_ops;
        initAVLTree(&tmp_tree_for_ops);
        // Travessia em ordem para popular tmp_tree_for_ops
        AVLNode** stack = (AVLNode**)malloc((tree->size + 1) * sizeof(AVLNode*)); 
        if (!stack) {
            perror("Erro ao alocar stack para travessia em ops combinadas");
            return;
        }
        int top = -1;
        AVLNode* current_original = tree->root;
        while (current_original != NULL || top != -1) {
            while (current_original != NULL) {
                stack[++top] = current_original;
                current_original = current_original->left;
            }
            current_original = stack[top--];
            insertAVLTree(&tmp_tree_for_ops, current_original->key, current_original->data);
            current_original = current_original->right;
        }
        free(stack);


        // Encontra o menor e maior UDI para o intervalo de operacoes
        AVLNode* minNode = tmp_tree_for_ops.root;
        while (minNode != NULL && minNode->left != NULL) {
            minNode = minNode->left;
        }
        AVLNode* maxNode = tmp_tree_for_ops.root;
        while (maxNode != NULL && maxNode->right != NULL) {
            maxNode = maxNode->right;
        }
        
        int minUDI = minNode ? minNode->key : 0; 
        int maxUDI = maxNode ? maxNode->key : 0; 

        if (maxUDI < minUDI) { 
            if (minNode) {
                maxUDI = minUDI + 1;
            } else {
                minUDI = 0; maxUDI = 1000000;
            }
        }

        start_timer(&t);
        for (int j = 0; j < num_operations_per_run; j++) {
            int op_type = rand() % 100; 
            if (op_type < 30) { // 30% Insercao
                Transaction d = {0}; 
                int new_udi = maxUDI + 1 + j; 
                snprintf(d.transaction_id, sizeof(d.transaction_id), "TRN%08d", new_udi);
                d.amount = 10.0f + (rand() % 1000) / 100.0f;
                insertAVLTree(&tmp_tree_for_ops, new_udi, d); 
                maxUDI = new_udi; 
            } else if (op_type < 80) { // 50% Busca
                if (maxUDI >= minUDI) { 
                    int search_udi = minUDI + (rand() % (maxUDI - minUDI + 1)); 
                    searchAVLTree(&tmp_tree_for_ops, search_udi); 
                }
            } else { // 20% Remocao
                if (tmp_tree_for_ops.size > 0) {
                    int remove_udi = minUDI + (rand() % (maxUDI - minUDI + 1)); 
                    deleteAVLTree(&tmp_tree_for_ops, remove_udi); 
                }
            }
        }
        results[i] = stop_timer(&t);
        freeAVLTree(tmp_tree_for_ops.root); 
    }

    double mean = calculate_mean(results, NUM_REPETITIONS);
    double std_dev = calculate_std_dev(results, NUM_REPETITIONS, mean);
    double cov = calculate_coeff_of_variation(mean, std_dev);

    printf("  Media: %.3f ms | DP: %.3f ms | CV: %.2f%%\n",
           mean, std_dev, cov);
}

// Executa todos os benchmarks completos para a AVLTree
void run_all_benchmarks(AVLTree* tree) {
    printf("\n===========================================\n");
    printf("=== INICIANDO BENCHMARKS COMPLETOS ===\n"); 
    printf("===========================================\n");

    // Limpa a arvore de benchmark antes de iniciar os testes
    freeAVLTree(tree->root);
    initAVLTree(tree);

    HighPrecisionTimer t_total;
    start_timer(&t_total);

    // Preenche a arvore com dados iniciais para os benchmarks de busca, remocao e acesso
    printf("\nPreparando arvore para benchmarks (inserindo 10.000 elementos base)... "); 
    generateRandomData(tree, 10000);
    printf("Concluido. Tamanho da arvore: %d\n", tree->size); 

    printf("\n1. Tempo de Insercao:\n"); 
    benchmark_insertion(tree, 1000);
    benchmark_insertion(tree, 10000);

    printf("\n2. Tempo de Remocao:\n"); 
    benchmark_removal(tree);

    printf("\n3. Tempo de Busca:\n"); 
    benchmark_search(tree);

    printf("\n4. Uso de Memoria:\n"); 
    estimate_memory_usage(tree);

    printf("\n5. Tempo Medio de Acesso:\n"); 
    benchmark_random_access(tree);

    printf("\n6. Escalabilidade:\n"); 
    benchmark_scalability();

    printf("\n7. Latencia Media (operacoes combinadas):\n"); 
    benchmark_combined_operations(tree);

    double elapsed_total = stop_timer(&t_total);
    printf("\nTempo TOTAL da suite de benchmarks completa: %.3f ms\n", elapsed_total);

    printf("\n===========================================\n");
    printf("=== BENCHMARKS CONCLUIDOS ===\n"); 
    printf("===========================================\n");
}

// Funcao para remover o primeiro elemento (para restricao R2)
void removeFirst(AVLTree* tree) {
    if (tree == NULL || tree->root == NULL) return;
    
    // Encontra o no com o menor UDI (mais a esquerda)
    AVLNode* current = tree->root;
    while (current->left != NULL) {
        current = current->left;
    }
    
    deleteAVLTree(tree, current->key);
}

// Gera dados, incluindo anomalias e aplicando restricoes de tempo/tamanho
void generateAnomalousData(AVLTree* tree, int count, int max_size) {
    for (int i = 0; i < count; i++) {
        // R2: Limitacao de tamanho
        if (max_size > 0 && tree->size >= max_size) {
            removeFirst(tree); 
        }

        // R10: Interrupcoes periodicas
        if (i % 100 == 0) {
            Sleep(1); 
        }

        Transaction d = {0};
        int udi_key = 10000 + i + rand() % 100000; // Garante UDI unico

        // Preenche os campos da transacao com dados aleatorios
        snprintf(d.transaction_id, sizeof(d.transaction_id), "TRN%08d", udi_key);
        snprintf(d.timestamp, sizeof(d.timestamp), "2024-01-01 %02d:%02d:%02d", rand() % 24, rand() % 60, rand() % 60);
        snprintf(d.sender_account, sizeof(d.sender_account), "ACC%05d", rand() % 10000);
        snprintf(d.receiver_account, sizeof(d.receiver_account), "ACC%05d", rand() % 10000);
        d.amount = 10.0f + (rand() % 10000) / 100.0f;
        char types[][16] = {"purchase", "transfer", "deposit", "withdrawal"};
        snprintf(d.transaction_type, sizeof(d.transaction_type), "%s", types[rand() % 4]);
        char categories[][32] = {"retail", "food", "travel", "online", "services"};
        snprintf(d.merchant_category, sizeof(d.merchant_category), "%s", categories[rand() % 5]);
        snprintf(d.location, sizeof(d.location), "City%d", rand() % 100);
        char devices[][16] = {"mobile", "web", "pos"};
        snprintf(d.device_used, sizeof(d.device_used), "%s", devices[rand() % 3]);
        d.is_fraud = rand() % 2;

        // R18: Insercao de anomalias (10% de chance de inserir dados anomalos)
        if (rand() % 10 == 0) {
            udi_key = -(1000000 + i); // UDI negativo para garantir unicidade e que se destacara
            d.amount = -999.0f;
            snprintf(d.transaction_type, sizeof(d.transaction_type), "ANOMALY"); 
        }

        insertAVLTree(tree, udi_key, d); // Insere na Arvore AVL

        // R13: Delay artificial por lote
        if (i > 0 && i % 100 == 0) {
            Sleep(50); 
        }
    }
}

void run_restricted_benchmarks() {
    printf("\n==============================================\n");
    printf("=== BENCHMARK COM RESTRICOES ATIVADAS ===\n"); 
    printf("==============================================\n");

    AVLTree tree;
    initAVLTree(&tree);

    HighPrecisionTimer t;
    start_timer(&t);

    // R2, R10, R13, R18
    generateAnomalousData(&tree, 1000, 500);

    double elapsed = stop_timer(&t);

    printf("\nTempo total (com 4 restricoes aplicadas): %.3f ms\n", elapsed);
    printf("Elementos finais na arvore (maximo 500): %d\n", tree.size); 

    // Benchmarks apos restricoes
    printf("\n--- Testes de Performance Pos-Restricoes ---\n"); 
    benchmark_search(&tree);
    benchmark_removal(&tree);
    benchmark_random_access(&tree);
    estimate_memory_usage(&tree);

    freeAVLTree(tree.root);

    printf("\n==============================================\n");
    printf("=== FIM DOS TESTES COM RESTRICOES ===\n"); 
    printf("==============================================\n");
}


// ================= MAIN FUNCTION (BENCHMARK ONLY) =================

int main() {
    // Inicializa o gerador de numeros aleatorios com o tempo atual
    srand((unsigned int)time(NULL));

    // Cria e inicializa uma AVLTree para benchmarks
    AVLTree benchmark_avl_tree;
    initAVLTree(&benchmark_avl_tree);

    int choice;

    do {
        printf("\n--- Opcoes de Benchmark (Arvore AVL) ---\n");
        printf("1. Rodar Benchmarks Completos\n");
        printf("2. Rodar Benchmarks Restritos\n");
        printf("3. Sair\n");
        printf("Escolha uma opcao: ");
        scanf("%d", &choice);

        // Limpa o buffer de entrada
        while (getchar() != '\n'); 

        switch (choice) {
            case 1:
                run_all_benchmarks(&benchmark_avl_tree);
                break;
            case 2:
                run_restricted_benchmarks();
                break;
            case 3:
                printf("Saindo do programa de benchmark.\n");
                break;
            default:
                printf("Opcao invalida. Por favor, tente novamente.\n");
                break;
        }
    } while (choice != 3);


    // Libera a arvore AVL principal de benchmark se ela ainda contiver dados
    freeAVLTree(benchmark_avl_tree.root);

    return 0;
}
