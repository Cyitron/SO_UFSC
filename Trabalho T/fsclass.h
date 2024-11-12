#ifndef fsclass_h
#define fsclass_h

#include <fstream>
#include <iostream>
#include <math.h>
#include <vector>
#include <memory>
#include <stdexcept>
#include <cstdint>

using namespace std;

class FileSystem
{
    private:
        fstream fsFile;

    public:
        string fsFileName;
        int blockSize;
        int qtdBlocks;
        int qtdInodes;
        int bitmapSize;
        int indexVectorSize;
        int blockVectorSize;
        std::unique_ptr<char[]> bitmap;
        std::unique_ptr<char[]> tmpDataBlock;


        FileSystem(const FileSystem &) = delete;
        FileSystem &operator=(const FileSystem &) = delete;

        ~FileSystem()
        {
            fsFile.close();  // `unique_ptr` gerencia bitmap e tmpDataBlock automaticamente
        }

        FileSystem(const std::string& fs_file_name) : fsFileName(fs_file_name)
        {
            loadFileSystem();
        }

        void createFileSystem(int block_size, int qtd_block, int qtd_node)
        {
            blockSize = block_size;
            qtdBlocks = qtd_block;
            qtdInodes = qtd_node;
            bitmapSize = static_cast<int>(ceil(static_cast<float>(qtd_block) / 8.0f));
            indexVectorSize = sizeof(INODE) * qtdInodes;
            blockVectorSize = qtdBlocks * blockSize;

            // Aloca ou realoca memória
            bitmap.reset(new char[bitmapSize]);
            tmpDataBlock.reset(new char[blockSize]);

            blankInit();        // Inicializa o arquivo com zeros
            writeHeader();      // Escreve o cabeçalho com as informações iniciais
            createDir("/");     // Cria o diretório raiz do sistema de arquivos

        }

        void loadSystemVariables()
        {
            // Posiciona o ponteiro de leitura no início do arquivo
            fsFile.seekg(0);

            // Variáveis temporárias para armazenar os valores lidos do arquivo
            std::int32_t tempBlockSize, tempQtdBlocks, tempQtdInodes;

            // lê valores diretamente em variáveis temporárias
            fsFile.read(reinterpret_cast<char*>(&tempBlockSize), sizeof(tempBlockSize));
            fsFile.read(reinterpret_cast<char*>(&tempQtdBlocks), sizeof(tempQtdBlocks));
            fsFile.read(reinterpret_cast<char*>(&tempQtdInodes), sizeof(tempQtdInodes));

            blockSize = tempBlockSize;
            qtdBlocks = tempQtdBlocks;
            qtdInodes = tempQtdInodes;

            // Cálculo das variáveis de tamanho
            bitmapSize = static_cast<int>(std::ceil(static_cast<float>(qtdBlocks) / 8.0f));
            indexVectorSize = sizeof(INODE) * qtdInodes;
            blockVectorSize = qtdBlocks * blockSize;

            bitmap.reset(new char[bitmapSize]);
            tmpDataBlock.reset(new char[blockSize]);
        }

        void addDir()
        {

        }

        void addFile()
        {

        }

        void remove()
        {

        }

        void move()
        {

        }

    private:
        void loadFileSystem()
        {
            // Tenta abrir o arquivo sem truncar
            fsFile.open(fsFileName, std::ios::out | std::ios::in | std::ios::binary);

            // Se não conseguiu abrir, tenta criar o arquivo
            if (!fsFile.is_open())
            {
                fsFile.clear(); // Limpa flags de erro
                fsFile.open(fsFileName, std::ios::out | std::ios::in | std::ios::binary | std::ios::trunc);
            }

            // Se o arquivo ainda não abriu, lança uma exceção
            if (!fsFile.is_open())
            {
                throw std::runtime_error("Erro: Não foi possível abrir ou criar o arquivo " + fsFileName);
            }
        }

        void writeHeader()
        {
            fsFile.seekp(0);

            fsFile.write(reinterpret_cast< char*>(&blockSize), sizeof(blockSize));
            fsFile.write(reinterpret_cast< char*>(&qtdBlocks), sizeof(qtdBlocks));
            fsFile.write(reinterpret_cast< char*>(&qtdInodes), sizeof(qtdInodes));
        }

        void blankInit()
        {
            int totalSize = 3 + bitmapSize + indexVectorSize + blockVectorSize;
            std::unique_ptr<char[]> zeroBuffer(new char[totalSize]{});

            fsFile.seekp(0);
            fsFile.write(zeroBuffer.get(), totalSize);
        }

        INODE createDirInode(string path)
        {
            INODE new_inode{};
            new_inode.IS_DIR = 1;   // Marca o inode como um diretório.
            new_inode.IS_USED = 1;  // Indica que o inode está em uso.
            new_inode.SIZE = 0;     // Define o tamanho do diretório como zero.
            new_inode = pathNameInode(new_inode, path);

            return new_inode;
        }

        INODE pathNameInode (INODE inode, std::string name)
        {
            std::fill(std::begin(inode.NAME), std::end(inode.NAME), 0);                 // Zera todo o campo NAME do inode

            for (size_t i = 0; i < std::min(static_cast<int>(name.size()), 10); i++)    // Copia o nome até o limite de 10 caracteres
            {
                inode.NAME[i] = name[i];
            }

            return inode;
        }

        void createDir(string path)
        {
            
        }

};

#endif