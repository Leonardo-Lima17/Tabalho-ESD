#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <cmath>
#include <map>
#include <vector>
#include <algorithm>
using namespace std;

const int TABLE_SIZE = 10000019;
const int MAX_RELOCATIONS = 500;

struct Transaction {
    char transaction_id[50];
    char timestamp[50];
    char sender_account[50];
    char receiver_account[50];
    float amount;
    char transaction_type[50];
    char merchant_category[50];
    char location[50];
    char device_used[50];
};

Transaction* table1[TABLE_SIZE];
Transaction* table2[TABLE_SIZE];

unsigned int hash1(const char* key) {
    unsigned int hash = 0;
    while (*key) hash = (hash * 31 + *key++) % TABLE_SIZE;
    return hash;
}

unsigned int hash2(const char* key) {
    unsigned int hash = 5381;
    while (*key) hash = ((hash << 5) + hash) + *key++;
    return hash % TABLE_SIZE;
}

bool insert(Transaction* trans) {
    Transaction* curr = trans;
    int count = 0;
    while (count < MAX_RELOCATIONS) {
        unsigned int index1 = hash1(curr->transaction_id);
        if (table1[index1] == nullptr) {
            table1[index1] = curr;
            return true;
        }
        swap(curr, table1[index1]);

        unsigned int index2 = hash2(curr->transaction_id);
        if (table2[index2] == nullptr) {
            table2[index2] = curr;
            return true;
        }
        swap(curr, table2[index2]);
        count++;
    }
    cout << "Falha ao inserir: muitas relocacoes.\n";
    delete curr;
    return false;
}

Transaction* search(const char* id) {
    unsigned int index1 = hash1(id);
    if (table1[index1] && strcmp(table1[index1]->transaction_id, id) == 0)
        return table1[index1];
    unsigned int index2 = hash2(id);
    if (table2[index2] && strcmp(table2[index2]->transaction_id, id) == 0)
        return table2[index2];
    
    cout << "Transacao com ID '" << id << "' nao encontrada.\n";
    return nullptr;
}

bool remove_transaction(const char* id) {
    unsigned int index1 = hash1(id);
    if (table1[index1] && strcmp(table1[index1]->transaction_id, id) == 0) {
        delete table1[index1];
        table1[index1] = nullptr;
        return true;
    }
    unsigned int index2 = hash2(id);
    if (table2[index2] && strcmp(table2[index2]->transaction_id, id) == 0) {
        delete table2[index2];
        table2[index2] = nullptr;
        return true;
    }
    return false;
}

void read_csv(const char* filename) {
    ifstream file(filename);
    string line;
    getline(file, line); // Cabeçalho
    while (getline(file, line)) {
        stringstream ss(line);
        string id, ts, sender, receiver, amt, type, category, loc, device;
        getline(ss, id, ',');
        getline(ss, ts, ',');
        getline(ss, sender, ',');
        getline(ss, receiver, ',');
        getline(ss, amt, ',');
        getline(ss, type, ',');
        getline(ss, category, ',');
        getline(ss, loc, ',');
        getline(ss, device, ',');

        Transaction* t = new Transaction;
        strncpy(t->transaction_id, id.c_str(), sizeof(t->transaction_id));
        strncpy(t->timestamp, ts.c_str(), sizeof(t->timestamp));
        strncpy(t->sender_account, sender.c_str(), sizeof(t->sender_account));
        strncpy(t->receiver_account, receiver.c_str(), sizeof(t->receiver_account));
        t->amount = amt.empty() ? 0.0f : stof(amt);
        strncpy(t->transaction_type, type.c_str(), sizeof(t->transaction_type));
        strncpy(t->merchant_category, category.c_str(), sizeof(t->merchant_category));
        strncpy(t->location, loc.c_str(), sizeof(t->location));
        strncpy(t->device_used, device.c_str(), sizeof(t->device_used));
        insert(t);
    }
    file.close();
}

void print_transaction(Transaction* t) {
    if (!t) return;
    cout << "ID: " << t->transaction_id << "\n";
    cout << "Data: " << t->timestamp << "\n";
    cout << "De: " << t->sender_account << "\n";
    cout << "Para: " << t->receiver_account << "\n";
    cout << "Valor: R$" << t->amount << "\n";
    cout << "Tipo: " << t->transaction_type << "\n";
    cout << "Categoria: " << t->merchant_category << "\n";
    cout << "Local: " << t->location << "\n";
    cout << "Dispositivo: " << t->device_used << "\n\n";
}

void calcular_estatisticas() {
    vector<float> valores;
    float soma = 0;
    int count = 0;
    float min_val = INFINITY, max_val = -INFINITY;

    for (int i = 0; i < TABLE_SIZE; ++i) {
        if (table1[i]) {
            float val = table1[i]->amount;
            valores.push_back(val);
            soma += val;
            min_val = min(min_val, val);
            max_val = max(max_val, val);
            count++;
        }
        if (table2[i]) {
            float val = table2[i]->amount;
            valores.push_back(val);
            soma += val;
            min_val = min(min_val, val);
            max_val = max(max_val, val);
            count++;
        }
    }

    if (count == 0) {
        cout << "Nenhuma transacao disponivel.\n";
        return;
    }

    float media = soma / count;

    float variancia = 0;
    for (float v : valores) variancia += pow(v - media, 2);
    float desvio = sqrt(variancia / count);

    sort(valores.begin(), valores.end());
    float mediana = (count % 2 == 0) ? 
        (valores[count/2 - 1] + valores[count/2]) / 2.0f : 
        valores[count/2];

    cout << "Total de Transacoes: " << count << "\n";
    cout << "Media: R$" << media << "\n";
    cout << "Desvio padrao: R$" << desvio << "\n";
    cout << "Valor minimo: R$" << min_val << "\n";
    cout << "Valor maximo: R$" << max_val << "\n";
    cout << "Mediana: R$" << mediana << "\n";
}

void agrupar_por_feature(const string& feature) {
    map<string, int> contagem;

    for (int i = 0; i < TABLE_SIZE; ++i) {
        auto processar = [&](Transaction* t) {
            if (!t) return;
            string chave;
            if (feature == "transaction_type") chave = t->transaction_type;
            else if (feature == "location") chave = t->location;
            else if (feature == "device_used") chave = t->device_used;
            else if (feature == "merchant_category") chave = t->merchant_category;
            else chave = "Desconhecido";
            contagem[chave]++;
        };
        processar(table1[i]);
        processar(table2[i]);
    }

    cout << "\nAgrupamento por '" << feature << "':\n";
    for (const auto& par : contagem)
        cout << par.first << ": " << par.second << " transacoes\n";
}

void menu() {
    int opcao;
    do {
        cout << "\n=== MENU ===\n";
        cout << "1. Inserir transacao\n";
        cout << "2. Buscar transacao\n";
        cout << "3. Remover transacao\n";
        cout << "4. Estatisticas\n";
        cout << "5. Agrupamento por tipo\n";
        cout << "6. Sair\n";
        cout << "Escolha: ";
        cin >> opcao;
        cin.ignore();

        char id[50];
        switch (opcao) {
            case 1: {
                Transaction* t = new Transaction;
                cout << "ID: "; cin.getline(t->transaction_id, 50);
                cout << "Data: "; cin.getline(t->timestamp, 50);
                cout << "De: "; cin.getline(t->sender_account, 50);
                cout << "Para: "; cin.getline(t->receiver_account, 50);
                cout << "Valor: "; cin >> t->amount; cin.ignore();
                cout << "Tipo: "; cin.getline(t->transaction_type, 50);
                cout << "Categoria: "; cin.getline(t->merchant_category, 50);
                cout << "Local: "; cin.getline(t->location, 50);
                cout << "Dispositivo: "; cin.getline(t->device_used, 50);
                insert(t);
                break;
            }
            case 2:
                cout << "Digite o ID para buscar: "; cin.getline(id, 50);
                print_transaction(search(id));
                break;
            case 3:
                cout << "Digite o ID para remover: "; cin.getline(id, 50);
                if (remove_transaction(id))
                    cout << "Removido com sucesso.\n";
                else
                    cout << "Transacao nao encontrada.\n";
                break;
            case 4:
                calcular_estatisticas();
                break;
            case 5: {
                cout << "Escolha a feature para agrupar:\n";
                cout << "1. Tipo de transacao (transaction_type)\n";
                cout << "2. Localizacao (location)\n";
                cout << "3. Dispositivo (device_used)\n";
                cout << "4. Categoria do comerciante (merchant_category)\n";
                int escolha;
                cin >> escolha;
                cin.ignore();
                string feature;
                switch (escolha) {
                   case 1: feature = "transaction_type"; break;
                   case 2: feature = "location"; break;
                   case 3: feature = "device_used"; break;
                   case 4: feature = "merchant_category"; break;
                   default: feature = ""; break;
                }
                if (!feature.empty())
                agrupar_por_feature(feature);
                else
                cout << "Opcao invalida.\n";
                break;
           }

            case 6:
                cout << "Encerrando...\n";
                break;
            default:
                cout << "Opcao invalida.\n";
        }
    } while (opcao != 6);
}

int main() {
    memset(table1, 0, sizeof(table1));
    memset(table2, 0, sizeof(table2));

    read_csv("C:\\Users\\leozi\\OneDrive\\Desktop\\TrabalhoESD.csv\\financial_fraud_detection_dataset.csv");
    menu();

    for (int i = 0; i < TABLE_SIZE; ++i) {
        if (table1[i]) delete table1[i];
        if (table2[i]) delete table2[i];
    }
    return 0;
}