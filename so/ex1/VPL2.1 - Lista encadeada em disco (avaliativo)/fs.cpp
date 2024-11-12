#ifndef fs_h
#define fs_h

#include <string>
#include <cstdio>
#include <cstring>


// 28 bytes
struct Nodo {
    int existeNodo; // 1 = sim; 0 = não
    char nome[20]; // 20 caracteres pro nome       
    int proximoNodoOffSet; // offSet do próximo nodo ou -1 caso não houver próximo nodo        
};

int buscaOffSetNome(FILE* arquivo, std::string& depoisDesteNome) {
	int primeiroNodoOffSet = 0;
	int offSetAux = 0; // var auxiliar utilizada pra iteração dos offSet's
	
	// os 4 primeiro bytes do arquivo indicam o offSet do primeiro nodo
	fread(&primeiroNodoOffSet, sizeof(int), 1, arquivo); // offSet do primeiro nodo
	offSetAux = primeiroNodoOffSet;
	
	while(offSetAux != -1) { // offSet -1 indica que não há mais nodos
		Nodo nodoAux;
		
		fseek(arquivo, offSetAux, SEEK_SET); // posiciona leitura à partir do offSetAux (primeiro nodo)
		fread(&nodoAux, sizeof(Nodo), 1, arquivo); // faz a leitura do nodo (28 bytes)
		
		if(nodoAux.nome == depoisDesteNome) {
			return offSetAux;
		}
		
		offSetAux = nodoAux.proximoNodoOffSet;
	}
	
	return -1;
}

int buscaBlocoDisponivel(FILE* arquivo) {
	int offSet = 0;
	int disp = 0;
	
	fseek(arquivo, 4, SEEK_SET);  // pula os primeiros 4 bytes do arquivo
	
	for(int i = 0; i < 10; i++) {
		offSet = 4 + i * sizeof(Nodo);
		fseek(arquivo, offSet, SEEK_SET); // posicionamento do ponteiro de leitura/esc
		fread(&disp, sizeof(int), 1, arquivo); // leitura dos 4 primeiros bytes (indicam se ta livre ou n)
		
		if(disp == 0) { // bloco está disponível pra escrita
			printf("here na funcao");
			return offSet;
		}
	}
	return -1; // se nenhum bloco estiver disponível
}

void adicionaNodo(FILE* arquivo, std::string& novoNome, int nodoAnteriorOffset, int blocoLivreOffset) {
	
    Nodo novoNodo;
    novoNodo.existeNodo = 1;  
    strncpy(novoNodo.nome, novoNome.c_str(), 20);  
    novoNodo.proximoNodoOffSet = -1;  

    Nodo nodoAnterior;
    fseek(arquivo, nodoAnteriorOffset, SEEK_SET);
    fread(&nodoAnterior, sizeof(Nodo), 1, arquivo);

    // atualiza o novo nodo para apontar para o próximo nodo do nodo anterior
    novoNodo.proximoNodoOffSet = nodoAnterior.proximoNodoOffSet	;

    // escreve o novo nodo no bloco livre encontrado
    fseek(arquivo, blocoLivreOffset, SEEK_SET);
    fwrite(&novoNodo, sizeof(Nodo), 1, arquivo);

    // atualiza o ponteiro do nodo anterior para apontar para o novo nodo
    fseek(arquivo, nodoAnteriorOffset + 24, SEEK_SET);
    fwrite(&blocoLivreOffset, sizeof(int), 1, arquivo);
}

/**
 * @param arquivoDaLista nome do arquivo em disco que contem a lista encadeada
 * @param novoNome nome a ser adicionado apos depoisDesteNome
 * @param depoisDesteNome um nome presente na lista
 */
void adiciona(std::string arquivoDaLista, std::string novoNome, std::string depoisDesteNome) {

    FILE* arquivo = fopen(arquivoDaLista.c_str(), "r+b");
    if(!arquivo) {
        printf("Erro ao abrir o arquivo");
        return;
    }

    int nodoAnteriorOffset = buscaOffSetNome(arquivo, depoisDesteNome);
    if(nodoAnteriorOffset == -1) {
        printf("Nome não encontrado");
        fclose(arquivo);
        return;
    } 

    int blocoLivreOffset = buscaBlocoDisponivel(arquivo);
    if(blocoLivreOffset == -1) {
        printf("Nao há blocos disponíveis");
        fclose(arquivo);
        return;     
    } 

    adicionaNodo(arquivo, novoNome, nodoAnteriorOffset, blocoLivreOffset);
	fclose(arquivo);
}

#endif /* fs_h */

