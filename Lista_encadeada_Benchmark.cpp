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
    struct Transaction* next; // Ponteiro para o proximo no na lista encadeada
} Transaction;

// Estrutura para a Lista Encadeada
typedef struct LinkedList {
    Transaction* head; // Ponteiro para o inicio da lista
    int size;          // Numero de elementos na lista
} LinkedList;

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


// Estrutura de um no da Arvore AVL (adicionada para o printf de sizeof(AVLNode))
// Esta estrutura e usada apenas para exibir seu tamanho na saida do benchmark,
// nao e parte da logica da Lista Encadeada.
typedef struct AVLNode {
    int key; 
    Transaction data;
    struct AVLNode* left;
    struct AVLNode* right;
    uint8_t height;
} AVLNode;

// ================= FUNCOES DA LISTA ENCADAEADA =================

// Inicializa a lista encadeada
void init_linked_list(LinkedList* list) {
    list->head = NULL;
    list->size = 0;
}

// Insere uma transacao no inicio da lista encadeada
void insert_transaction_linked_list(LinkedList* list, Transaction t) {
    Transaction* new_node = (Transaction*)malloc(sizeof(Transaction));
    if (!new_node) {
        fprintf(stderr, "Erro de alocacao de memoria para novo no.\n");
        exit(EXIT_FAILURE);
    }
    *new_node = t; 
    new_node->next = list->head; 
    list->head = new_node;
    list->size++; 
}

// Busca uma transacao na lista encadeada pelo transaction_id
Transaction* search_transaction_linked_list(LinkedList* list, const char* transaction_id) {
    Transaction* current = list->head;
    while (current) {
        if (strcmp(current->transaction_id, transaction_id) == 0)
            return current;
        current = current->next;
    }
    return NULL;
}

// Remove uma transacao da lista encadeada pelo transaction_id
void remove_transaction_linked_list(LinkedList* list, const char* transaction_id) {
    Transaction* current = list->head;
    Transaction* prev = NULL;

    while (current) {
        if (strcmp(current->transaction_id, transaction_id) == 0) {
            if (prev) {
                prev->next = current->next;
            } else {
                list->head = current->next;
            }
            free(current);
            list->size--;
            return;
        }
        prev = current;
        current = current->next;
    }
}

// Libera toda a memoria alocada pela lista encadeada
void free_linked_list(LinkedList* list) {
    Transaction* current = list->head;
    while (current) {
        Transaction* temp = current;
        current = current->next;
        free(temp);
    }
    list->head = NULL;
    list->size = 0;
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

// Gera dados aleatorios e os insere na Lista Encadeada
void generateRandomDataForLinkedList(LinkedList* list, int count) {
    for (int i = 0; i < count; i++) {
        Transaction d = {0}; // Usa a estrutura Transaction
        // Gera transaction_id unico como string
        snprintf(d.transaction_id, sizeof(d.transaction_id), "TRN%08d%04d", i, rand() % 10000); 
        
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

        insert_transaction_linked_list(list, d);
    }
}

// Benchmark de insercao da Lista Encadeada
void benchmark_insertion_linked_list(int num_elements) {
    double results[NUM_REPETITIONS];
    printf("\nBenchmark Insercao (%d elementos):\n", num_elements);

    for (int i = 0; i < NUM_REPETITIONS; i++) {
        HighPrecisionTimer t;
        LinkedList tmp_list;
        init_linked_list(&tmp_list); // Sempre inicia com uma lista limpa
        
        start_timer(&t);
        generateRandomDataForLinkedList(&tmp_list, num_elements);
        results[i] = stop_timer(&t);
        
        free_linked_list(&tmp_list); // Libera memoria da lista temporaria
    }

    double mean = calculate_mean(results, NUM_REPETITIONS);
    double std_dev = calculate_std_dev(results, NUM_REPETITIONS, mean);
    double cov = calculate_coeff_of_variation(mean, std_dev);

    printf("  Media: %.3f ms\n", mean);
    printf("  Desvio Padrao: %.3f ms\n", std_dev);
    printf("  Coeficiente de Variacao: %.2f%%\n", cov);
}

// Benchmark de busca da Lista Encadeada
void benchmark_search_linked_list(LinkedList* list) {
    if (list->size == 0) {
        printf("Lista vazia para busca\n");
        return;
    }

    double results[NUM_REPETITIONS];
    printf("\nBenchmark Busca (%d ops):\n", 10000); // 10000 ops e o numero de buscas por repeticao

    // Coleta as transaction_ids existentes para buscar (fora do loop de repeticoes para consistencia)
    char** ids_to_search = (char**)malloc(list->size * sizeof(char*));
    if (!ids_to_search) {
        perror("Falha ao alocar memoria para IDs de busca");
        return;
    }
    int id_idx = 0;
    Transaction* current_node = list->head;
    while (current_node) {
        if (id_idx < list->size) {
            ids_to_search[id_idx] = (char*)malloc(sizeof(current_node->transaction_id));
            strcpy(ids_to_search[id_idx], current_node->transaction_id);
            id_idx++;
        }
        current_node = current_node->next;
    }

    for (int i = 0; i < NUM_REPETITIONS; i++) {
        HighPrecisionTimer t;
        const int searches_per_run = 10000;
        int found = 0;
        
        start_timer(&t);
        for (int j = 0; j < searches_per_run; j++) {
            // Busca por um ID aleatorio do conjunto de IDs existentes
            const char* search_id = ids_to_search[rand() % list->size];
            if (search_transaction_linked_list(list, search_id) != NULL) {
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

    // Libera a memoria dos IDs coletados
    for (int i = 0; i < id_idx; i++) {
        free(ids_to_search[i]);
    }
    free(ids_to_search);
}

// Benchmark de remocao da Lista Encadeada
void benchmark_removal_linked_list(LinkedList* list) {
    if (list->size == 0) {
        printf("Lista vazia para remocao\n");
        return;
    }

    double results[NUM_REPETITIONS];
    printf("\nBenchmark Remocao (%d ops):\n", 1000); // 1000 ops e o numero de remocoes por repeticao

    for (int i = 0; i < NUM_REPETITIONS; i++) {
        HighPrecisionTimer t;
        LinkedList tmp_list;
        init_linked_list(&tmp_list);

        // Copia elementos da lista original para uma lista temporaria para remocao
        Transaction* current_original = list->head;
        while (current_original) {
            insert_transaction_linked_list(&tmp_list, *current_original); 
            current_original = current_original->next;
        }

        const int removals_per_run = (tmp_list.size < 1000) ? tmp_list.size / 2 : 1000;
        if (removals_per_run == 0 && tmp_list.size > 0) { // Garante pelo menos 1 remocao se houver elementos
             // Nao ha remocoes suficientes para 1000 ops, ajusta.
        }

        // Coleta as transaction_ids para remocao da tmp_list
        char** ids_to_remove = (char**)malloc(tmp_list.size * sizeof(char*));
        if (!ids_to_remove) {
            perror("Falha ao alocar memoria para IDs de remocao");
            free_linked_list(&tmp_list);
            return;
        }
        int id_idx = 0;
        Transaction* current_tmp = tmp_list.head;
        while (current_tmp) {
            if (id_idx < tmp_list.size) { 
                ids_to_remove[id_idx] = (char*)malloc(sizeof(current_tmp->transaction_id));
                strcpy(ids_to_remove[id_idx], current_tmp->transaction_id);
                id_idx++;
            }
            current_tmp = current_tmp->next;
        }
        // Embaralha os IDs para remocao aleatoria
        for (int k = id_idx - 1; k > 0; k--) {
            int j = rand() % (k + 1);
            char* temp = ids_to_remove[k];
            ids_to_remove[k] = ids_to_remove[j];
            ids_to_remove[j] = temp;
        }

        start_timer(&t);
        for (int j = 0; j < removals_per_run; j++) {
            if (tmp_list.size > 0 && j < id_idx) { 
                remove_transaction_linked_list(&tmp_list, ids_to_remove[j]);
            }
        }
        results[i] = stop_timer(&t);

        // Libera a memoria dos IDs coletados
        for (int k = 0; k < id_idx; k++) {
            free(ids_to_remove[k]);
        }
        free(ids_to_remove);
        free_linked_list(&tmp_list); // Libera a lista temporaria apos cada repeticao
    }

    double mean = calculate_mean(results, NUM_REPETITIONS);
    double std_dev = calculate_std_dev(results, NUM_REPETITIONS, mean);
    double cov = calculate_coeff_of_variation(mean, std_dev);

    printf("  Media: %.3f ms\n", mean);
    printf("  Desvio Padrao: %.3f ms\n", std_dev);
    printf("  Coeficiente de Variacao: %.2f%%\n", cov);
}

// Calcula o tamanho em bytes de um no da Lista Encadeada
size_t calculate_node_size_linked_list() {
    return sizeof(Transaction);
}

// Estima o uso total de memoria da Lista Encadeada
void estimate_memory_usage_linked_list(LinkedList* list) {
    if (list == NULL) {
        printf("Lista invalida\n");
        return;
    }
    
    size_t node_size = calculate_node_size_linked_list();
    size_t total_memory = list->size * node_size + sizeof(LinkedList);

    printf("\n=== ESTIMATIVA DE USO DE MEMORIA ===\n");
    printf("Tamanho por no: %zu bytes\n", node_size);
    printf("Numero de nos: %d\n", list->size);
    printf("Memoria total estimada: %zu bytes (%.2f KB)\n",
           total_memory, (float)total_memory / 1024);

    printf("\nComparacao com sizeof:\n");
    printf("sizeof(MachineData): %zu bytes\n", sizeof(Transaction)); 
    printf("sizeof(AVLNode): %zu bytes\n", sizeof(AVLNode)); 
    printf("Obs: Pode haver padding/alignment pelo compilador\n");
}

// Benchmark de acesso aleatorio da Lista Encadeada
void benchmark_random_access_linked_list(LinkedList* list) {
    if (list->size == 0) {
        printf("Lista vazia para teste de acesso aleatorio\n");
        return;
    }

    double results[NUM_REPETITIONS];
    printf("\nBenchmark Acesso Aleatorio (%d acessos):\n", 10000); // 10000 acessos por repeticao

    // Coleta as transaction_ids existentes para acesso aleatorio (fora do loop de repeticoes)
    char** ids_for_access = (char**)malloc(list->size * sizeof(char*));
    if (!ids_for_access) {
        perror("Falha ao alocar memoria para IDs de acesso");
        return;
    }
    int id_idx = 0;
    Transaction* current_node = list->head;
    while (current_node) {
        if (id_idx < list->size) {
            ids_for_access[id_idx] = (char*)malloc(sizeof(current_node->transaction_id));
            strcpy(ids_for_access[id_idx], current_node->transaction_id);
            id_idx++;
        }
        current_node = current_node->next;
    }

    for (int i = 0; i < NUM_REPETITIONS; i++) {
        HighPrecisionTimer t;
        const int accesses_per_run = 10000;
        long long sum = 0; 

        start_timer(&t);
        for (int j = 0; j < accesses_per_run; j++) {
            const char* random_id = ids_for_access[rand() % list->size];
            Transaction* found_tx = search_transaction_linked_list(list, random_id); 
            if (found_tx != NULL) {
                sum += (long long)found_tx->transaction_id[0]; 
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

    // Libera a memoria dos IDs coletados
    for (int i = 0; i < id_idx; i++) {
        free(ids_for_access[i]);
    }
    free(ids_for_access);
}

// Benchmark de escalabilidade da Lista Encadeada para insercao
void benchmark_scalability_linked_list() {
    printf("\nBenchmark de Escalabilidade:\n");
    int sizes[] = {1000, 5000, 10000, 20000, 50000, 100000};
    int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < num_sizes; s++) { // Loop pelos tamanhos
        int current_size = sizes[s];
        double results[NUM_REPETITIONS];

        for (int i = 0; i < NUM_REPETITIONS; i++) { // Repeticoes para cada tamanho
            LinkedList list;
            init_linked_list(&list);

            HighPrecisionTimer t;
            start_timer(&t);
            generateRandomDataForLinkedList(&list, current_size);
            results[i] = stop_timer(&t);
            free_linked_list(&list);
        }
        double mean = calculate_mean(results, NUM_REPETITIONS);
        double std_dev = calculate_std_dev(results, NUM_REPETITIONS, mean);
        double cov = calculate_coeff_of_variation(mean, std_dev);

        printf("  Tamanho: %6d elementos | Media: %7.3f ms | DP: %7.3f ms | CV: %.2f%%\n",
               current_size, mean, std_dev, cov);
    }
}

// Benchmark de operacoes combinadas (insercao, busca, remocao) na Lista Encadeada
void benchmark_combined_operations_linked_list(LinkedList* list) {
    double results[NUM_REPETITIONS];
    printf("\nBenchmark Operacoes Combinadas (%d ops):\n", 1000); // 1000 ops por repeticao

    for (int i = 0; i < NUM_REPETITIONS; i++) {
        HighPrecisionTimer t;
        const int num_operations_per_run = 1000;
        
        // Crie uma copia da lista para cada repeticao para garantir o estado inicial
        LinkedList tmp_list_for_ops;
        init_linked_list(&tmp_list_for_ops);
        Transaction* current_original = list->head;
        while(current_original) {
            insert_transaction_linked_list(&tmp_list_for_ops, *current_original);
            current_original = current_original->next;
        }

        start_timer(&t);
        for (int j = 0; j < num_operations_per_run; j++) {
            int op_type = rand() % 100; // 0-99
            if (op_type < 30) { // 30% Insercao
                Transaction d = {0};
                snprintf(d.transaction_id, sizeof(d.transaction_id), "TRN%08d%04d", j + (i * num_operations_per_run) + 1000000, rand() % 10000); 
                d.amount = 10.0f + (rand() % 10000) / 100.0f;
                insert_transaction_linked_list(&tmp_list_for_ops, d);
            } else if (op_type < 80) { // 50% Busca (30-79)
                char search_id[16];
                snprintf(search_id, sizeof(search_id), "TRN%08d%04d", rand() % (1000000 + num_operations_per_run), rand() % 10000); 
                search_transaction_linked_list(&tmp_list_for_ops, search_id);
            } else { // 20% Remocao (80-99)
                if (tmp_list_for_ops.size > 0) {
                    // Para remocao de um elemento que provavelmente existe
                    // pegue um ID aleatorio dos elementos *atualmente* na lista temporaria.
                    // Isso e mais complexo em uma lista encadeada; para simplicidade,
                    // vamos tentar remover um ID aleatorio do pool original ou um recem-inserido.
                    char remove_id[16];
                    snprintf(remove_id, sizeof(remove_id), "TRN%08d%04d", rand() % (1000000 + num_operations_per_run), rand() % 10000); 
                    remove_transaction_linked_list(&tmp_list_for_ops, remove_id);
                }
            }
        }
        results[i] = stop_timer(&t);
        free_linked_list(&tmp_list_for_ops); // Libera a lista temporaria
    }

    double mean = calculate_mean(results, NUM_REPETITIONS);
    double std_dev = calculate_std_dev(results, NUM_REPETITIONS, mean);
    double cov = calculate_coeff_of_variation(mean, std_dev);

    printf("  Media: %.3f ms | DP: %.3f ms | CV: %.2f%%\n",
           mean, std_dev, cov);
}

// Executa todos os benchmarks completos para a Lista Encadeada
void run_all_benchmarks_linked_list(LinkedList* list_ptr) { 
    printf("\n===========================================\n");
    printf("=== INICIANDO BENCHMARKS COMPLETOS ===\n"); 
    printf("===========================================\n");

    // Limpa e inicializa a lista antes de cada execucao completa
    free_linked_list(list_ptr); 
    init_linked_list(list_ptr); 

    // Preenche a lista com dados iniciais para os benchmarks de busca, remocao e acesso
    printf("\nPreparando arvore para benchmarks (inserindo 10.000 elementos)... "); 
    generateRandomDataForLinkedList(list_ptr, 10000); 
    printf("Concluido. Tamanho da arvore: %d\n", list_ptr->size); 

    printf("\n1. Tempo de Insercao:\n"); 
    benchmark_insertion_linked_list(1000); 
    benchmark_insertion_linked_list(10000); 

    printf("\n2. Tempo de Remocao:\n"); 
    benchmark_removal_linked_list(list_ptr); 

    printf("\n3. Tempo de Busca:\n"); 
    benchmark_search_linked_list(list_ptr); 

    printf("\n4. Uso de Memoria:\n"); 
    estimate_memory_usage_linked_list(list_ptr); 

    printf("\n5. Tempo Medio de Acesso:\n"); 
    benchmark_random_access_linked_list(list_ptr); 

    printf("\n6. Escalabilidade:\n"); 
    benchmark_scalability_linked_list();

    printf("\n7. Latencia Media (operacoes combinadas):\n"); 
    benchmark_combined_operations_linked_list(list_ptr); 

    printf("\n===========================================\n");
    printf("=== BENCHMARKS CONCLUIDOS ===\n"); 
    printf("===========================================\n");
}

// Funcao para remover o primeiro elemento (para restricao R2)
void remove_first_linked_list(LinkedList* list) {
    if (list == NULL || list->head == NULL) return;
    
    Transaction* temp = list->head;
    list->head = list->head->next;
    free(temp);
    list->size--;
}

// Gera dados, incluindo anomalias e aplicando restricoes de tempo/tamanho
void generateAnomalousDataForLinkedList(LinkedList* list, int count, int max_size) {
    for (int i = 0; i < count; i++) {
        // R2: Limitacao de tamanho (se a lista atingir max_size, remove o primeiro elemento)
        if (max_size > 0 && list->size >= max_size) {
            remove_first_linked_list(list);
        }

        // R10: Interrupcoes periodicas (simula pausas a cada 100 iteracoes)
        if (i > 0 && i % 100 == 0) {
            Sleep(1); // Pausa de 1 milissegundo
        }

        Transaction d = {0}; // Usa a estrutura Transaction
        // Gera transaction_id unico como string
        snprintf(d.transaction_id, sizeof(d.transaction_id), "TRN%08d%04d", i + 2000000, rand() % 10000); 
        
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
            snprintf(d.transaction_id, sizeof(d.transaction_id), "ANOMALY%06d", i + 1); // ID anomalo
            d.amount = -999.0f; // Valor anomalo
            snprintf(d.transaction_type, sizeof(d.transaction_type), "ANOMALY"); // Tipo anomalo
            d.is_fraud = true; // Marcar como fraude
        }

        insert_transaction_linked_list(list, d); // Insere na Lista Encadeada

        // R13: Delay artificial por lote (pausa a cada 100 transacoes inseridas)
        if (i > 0 && i % 100 == 0) {
            Sleep(50); // Pausa de 50 milissegundos
        }
    }
}

// Funcao para executar os benchmarks com restricoes para a Lista Encadeada
void run_restricted_benchmarks_linked_list() {
    printf("\n==============================================\n");
    printf("=== BENCHMARK COM RESTRICOES ATIVADAS ===\n"); 
    printf("==============================================\n");

    LinkedList list; 
    init_linked_list(&list); 

    HighPrecisionTimer t;
    start_timer(&t);

    // R2, R10, R13, R18
    generateAnomalousDataForLinkedList(&list, 1000, 500); 

    double elapsed = stop_timer(&t);

    printf("\nTempo total (com 4 restricoes aplicadas): %.3f ms\n", elapsed);
    printf("Elementos finais na arvore (maximo 500): %d\n", list.size); 

    // Benchmarks apos restricoes
    printf("\n--- Testes de Performance Pos-Restricoes ---\n"); 
    benchmark_search_linked_list(&list); 
    benchmark_removal_linked_list(&list); 
    benchmark_random_access_linked_list(&list); 
    estimate_memory_usage_linked_list(&list); 

    free_linked_list(&list); 
    printf("\n==============================================\n");
    printf("=== FIM DOS TESTES COM RESTRICOES ===\n"); 
    printf("==============================================\n");
}


// ================= MAIN FUNCTION (BENCHMARK ONLY) =================

int main() {
    // Inicializa o gerador de numeros aleatorios com o tempo atual
    srand((unsigned int)time(NULL));

    // Cria e inicializa uma LinkedList para benchmarks
    LinkedList benchmark_linked_list; 
    init_linked_list(&benchmark_linked_list); 

    int choice;

    do {
        printf("\n--- Opcoes de Benchmark (Lista Encadeada) ---\n"); 
        printf("1. Rodar Benchmarks Completos\n");
        printf("2. Rodar Benchmarks Restritos\n");
        printf("3. Sair\n");
        printf("Escolha uma opcao: ");
        scanf("%d", &choice);

        // Limpa o buffer de entrada
        while (getchar() != '\n'); 

        switch (choice) {
            case 1:
                run_all_benchmarks_linked_list(&benchmark_linked_list); 
                break;
            case 2:
                run_restricted_benchmarks_linked_list(); 
                break;
            case 3:
                printf("Saindo do programa de benchmark.\n");
                break;
            default:
                printf("Opcao invalida. Por favor, tente novamente.\n");
                break;
        }
    } while (choice != 3);

    // Libera a lista encadeada principal de benchmark se ela ainda contiver dados
    free_linked_list(&benchmark_linked_list); 

    return 0;
}
