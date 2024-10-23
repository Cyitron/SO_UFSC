#ifndef fs_h
#define fs_h
#include <string>
#include <fstream>
#include <iostream>
#include <cstring>

typedef struct {
    char header[4];
    char content[20];
    char next_header[4];
} Binary_Struct;

/**
 * @param arquivoDaLista nome do arquivo em disco que contem a lista encadeada
 * @param novoNome nome a ser adicionado apos depoisDesteNome
 * @param depoisDesteNome um nome presente na lista
 */
void adiciona(std::string arquivoDaLista, std::string novoNome, std::string depoisDesteNome)
{
    std::fstream file;
    file.open(arquivoDaLista, std::ios::in | std::ios::out | std::ios::binary);
    if(!file.is_open())
    {
        std::cout << "falha ao abrir o arquivo" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    int primeiroNode;

    file.read(reinterpret_cast<char*>(&primeiroNode), sizeof(primeiroNode));

    int posAtual = primeiroNode;
    Binary_Struct NewNode;
    int posAnterior = -1;

    while(posAtual != -1)

    while()

    Binary_Struct obj{{}};
    
    file.close();
};

#endif /* fs_h */