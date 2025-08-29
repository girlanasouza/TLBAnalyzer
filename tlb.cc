#include <iostream>
#include <iomanip>
#include <cstdint>
#include <list>
#include <unordered_map> // busca rápida no TLB
#include <unordered_set> // para verificar páginas já vistas

using namespace std;

class TLBSimulator {
private:
    size_t tlb_size; 
    uint64_t page_size; 
    
    list<uint64_t> tlb_lru_order; // ordem LRU (mais recente na frente)
    unordered_map<uint64_t, list<uint64_t>::iterator> tlb; 
    unordered_set<uint64_t> seen_pages; 
    
    long long accesses;
    long long hits;
    long long misses;
    long long cold_misses;
    long long capacity_misses;

public:
    TLBSimulator(size_t tlb_size, uint64_t page_size):
        tlb_size(tlb_size), 
        page_size(page_size),
        accesses(0),
        hits(0),
        misses(0),
        cold_misses(0),
        capacity_misses(0) {}

    void access_address(uint64_t address) {
        accesses++;
        uint64_t page_number = address / page_size;
        auto it = tlb.find(page_number);

        // TLB hit
        if (it != tlb.end()) {
            hits++;
            // move para frente (mais recentemente usado)
            tlb_lru_order.splice(tlb_lru_order.begin(), tlb_lru_order, it->second);
            return;
        }

        // TLB miss
        misses++;  // <-- CORRIGIDO: sempre conta um miss

        if (seen_pages.find(page_number) == seen_pages.end()) {
            cold_misses++;
            seen_pages.insert(page_number);
        } else {
            capacity_misses++;
        }

        // TLB cheia → remove LRU
        if (tlb.size() >= tlb_size) {
            uint64_t page_to_evict = tlb_lru_order.back();
            tlb.erase(page_to_evict);
            tlb_lru_order.pop_back();
        }

        // adiciona nova página
        tlb_lru_order.push_front(page_number);
        tlb[page_number] = tlb_lru_order.begin();
    }

    void print_stats() {
        cout << "Accesses: " << accesses << endl;
        cout << "Hits: " << hits << endl;
        cout << "Misses: " << misses << endl;
        cout << "  Cold Misses: " << cold_misses << endl;
        cout << "  Capacity Misses: " << capacity_misses << endl;

        double hit_rate = (accesses > 0) ? (static_cast<double>(hits) / accesses) * 100.0 : 0.0;
        cout << fixed << setprecision(2);
        cout << "Hit Rate: " << hit_rate << "%" << endl;
    }

    const unordered_set<uint64_t>& get_seen_pages() const {
        return seen_pages;
    }
};

std::unordered_set<uint64_t> run_experiment(size_t matrix_dim, size_t tlb_size, uint64_t page_size) {
    std::cout << "\nIniciando experimento: Matriz " << matrix_dim << "x" << matrix_dim 
              << ", TLB com " << tlb_size << " entradas, Página de " << page_size << " bytes" << std::endl;
    
    TLBSimulator simulator(tlb_size, page_size);

    int* matrix = new int[matrix_dim * matrix_dim];

    // Simula o acesso à matriz linha por linha (Row-Major Order)
    for (size_t i = 0; i < matrix_dim; ++i) {
        for (size_t j = 0; j < matrix_dim; ++j) {
            // Obtém o endereço do elemento matrix[i][j]
            int& element = matrix[i * matrix_dim + j];
            
            // Converte o ponteiro (endereço) para um inteiro de 64 bits
            uint64_t virtual_address = reinterpret_cast<uint64_t>(&element);

            // Alimenta o simulador com o endereço real
            simulator.access_address(virtual_address);
        }
    }
            
    simulator.print_stats();
    
    // Libera a memória alocada para a matriz para evitar vazamentos
    delete[] matrix;
    
    return simulator.get_seen_pages();
}

// --- Exemplo de uso ---
int main() {
    // Experimento 1: Matriz pequena, onde os dados podem caber em poucas páginas
    run_experiment(/*matrix_dim=*/ 1024, /*tlb_size=*/ 64, /*page_size=*/ 4096);
    
    // Experimento 2: Matriz maior, que provavelmente causará mais misses de capacidade
    run_experiment(/*matrix_dim=*/ 512, /*tlb_size=*/ 64, /*page_size=*/ 4096);

    return 0;
}