Todas as tarefas foram implementadas em conjunto pelos integrantes
Prompt de Comandos:

-Hash Table

1. (Explique como ler aqrquivo CSV em linguagem C)
//Código:
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

2. (Faça uma função para inserção de dados na hash table)
   //Código:
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
}

3.  (Faça uma função para remoção dos dados do dataset na hash table)
   //Código:
   void remove_transaction(const char* transaction_id) {
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

4. (Faça uma função para busca de um dado do dataset por ID na hash table)
   Transaction* search_transaction(const char* transaction_id) {
    unsigned int index = hash(transaction_id);
    Transaction* current = hash_table[index];
    while (current) {
        if (strcmp(current->transaction_id, transaction_id) == 0)
            return current;
        current = current->next;
    }
    return NULL;
}

5. (Faça uma otimização com bitfields para essa Arvore AVL)
   typedef struct AVLNode {
    Transaction data;
    struct AVLNode* left;
    struct AVLNode* right;
    uint8_t height;
} AVLNode;

6. (Essa é uma hash table..Me ajude a fazer uma função de benchmark que aborde as seguintes métricas : tempo de inserção, tempo de remoção, tempo de busca, uso de memória, tempo médio de acesso(escalabilidade) e latencia média para inserção, busca e remoção)
    

