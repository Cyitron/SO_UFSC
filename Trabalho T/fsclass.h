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

        void addDir(const std::string& full_path)
        {
            std::string father_dir_name = getFatherDirName(full_path);
            std::string dir_name = full_path.substr(full_path.find_last_of("/") + 1);

            createDir(dir_name);

            addFileToDir(father_dir_name, dir_name);
        }

        void addFile(const std::string& fullPath, const std::string& content)
        {
            std::string fatherDirName = getFatherDirName(fullPath);
            std::string fileName = fullPath.substr(fullPath.find_last_of('/') + 1);

            // Cria o arquivo no sistema
            createFile(fileName);

            // Adiciona o arquivo ao diretório pai
            addFileToDir(fatherDirName, fileName);

            // Preenche o arquivo com o conteúdo fornecido
            writeFile(fileName, content);
        }

        void remove(const std::string& full_path)
        {
            // Obtém o nome do diretório pai e o nome do arquivo/diretório
            std::string fatherDirName = getFatherDirName(full_path);
            std::string fileName = full_path.substr(full_path.find_last_of("/") + 1);

            // Busca os índices dos inodes e valida-os
            int fatherDirInodeIndex = getInodeIndex(fatherDirName);
            if (fatherDirInodeIndex < 0)
            {
                throw std::runtime_error("Parent directory inode not found: " + fatherDirName);
            }

            int fileInodeIndex = getInodeIndex(fileName);
            if (fileInodeIndex < 0)
            {
                throw std::runtime_error("File inode not found: " + fileName);
            }

            // Carrega os inodes
            INODE fatherDirInode = getInodeAtIndex(fatherDirInodeIndex);
            INODE fileInode = getInodeAtIndex(fileInodeIndex);

            // Libera os blocos diretos do inode do arquivo
            for (int i = 0; i < std::size(fileInode.DIRECT_BLOCKS); i++)
            {
                if (fileInode.DIRECT_BLOCKS[i] != 0)
                {
                    freeBitMap(static_cast<int>(fileInode.DIRECT_BLOCKS[i]));
                }
            }

            // Zera e reescreve o inode do arquivo
            writeInode(fileInodeIndex, INODE{});

            // Atualiza o tamanho do diretório pai
            if (fatherDirInode.SIZE > 0)
            {
                fatherDirInode.SIZE--;
            }

            // Defragmenta o diretório pai para manter a consistência
            defragmentDir(fatherDirInodeIndex, fatherDirInode);

            // Escreve o inode atualizado do diretório pai
            writeInode(fatherDirInodeIndex, fatherDirInode);
        }


        void move(string old_full_path, string new_full_path)
        {

            int a;

            // pega o nome do arquivo
            string old_file_name = old_full_path.substr(old_full_path.find_last_of("/") + 1, old_full_path.size());

            string file_name = new_full_path.substr(new_full_path.find_last_of("/") + 1, new_full_path.size());

            // pega o nome do novo pai
            string new_father_dir_name = this->getFatherDirName(new_full_path);
            // pega o nome do pai antigo
            string old_father_dir_name = this->getFatherDirName(old_full_path);

            // pega o inode do arquivo
            int file_inode_index = this->getInodeIndex(old_file_name);
            INODE file_inode = this->getInodeAtIndex(file_inode_index);

            if (old_file_name != file_name)
            {
                // cout << "O nome do arquivo mudou" << endl;
                // troca o nome do arquivo no inode
                file_inode = this->pathNameInode(file_inode, file_name);
                // salva o inode no arquivo
                this->writeInode(file_inode_index, file_inode);
                // cout << "nome do arquivo trocado" << endl;
            }

            // pega o conteudo do arquivo
            // cout << "pegando o conteudo do arquivo" << endl;
            string file_content = this->getFileContent(file_inode);
            // cout << "file_content: " << file_content << endl;

            // insere o indice do inode do arquivo no novo pai
            // cout << "inserindo o indice do inode do arquivo no novo pai" << endl;
            this->addFileToDir(new_father_dir_name, file_name);

            // remove o indice do inode do arquivo do pai antigo

            // cout << "removendo o indice do inode do arquivo do pai antigo" << endl;
            this->removeFileRefFromDir(old_father_dir_name, file_inode_index);
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

        INODE pathNameInode (INODE inode, std::string name)
        {
            std::fill(std::begin(inode.NAME), std::end(inode.NAME), 0);                 // Zera todo o campo NAME do inode

            for (size_t i = 0; i < std::min(static_cast<int>(name.size()), 10); i++)
            {
                inode.NAME[i] = name[i];
            }

            return inode;
        }

        INODE createFileInode (string file_name)
        {
            INODE new_inode{};
            new_inode.IS_USED = 0x01;
            new_inode.IS_DIR = 0x00;
            new_inode.SIZE = 0x00;

            new_inode = pathNameInode(new_inode, file_name);

            return new_inode;
        }

        INODE createDirInode(string path)
        {
            INODE new_inode{};
            new_inode.IS_DIR = 0x01;   // Marca o inode como um diretório.
            new_inode.IS_USED = 0x01;  // Indica que o inode está em uso.
            new_inode.SIZE = 0x00;     // Define o tamanho do diretório como zero.
            new_inode = pathNameInode(new_inode, path);

            return new_inode;
        }

        void getBitMap ()
        {
            if (!bitmap)
            {
                return;
            }
            this->fsFile.seekg(3);
            this->fsFile.read(bitmap.get(), this->bitmapSize);

        }

        int firstFreeBlockBitMap()
        {
            this->getBitMap();
            int index_livre = 0;
            // descobre a posição do primeiro bit 0 do bitmap
            for (int i = 0; i < this->bitmapSize; i++)
            {
                for (int j = 0; j < 8; j++)
                {
                    if ((this->bitmap[i] & (1 << j)) == 0)
                    {
                        index_livre = i * 4 + j;
                        return index_livre;
                    }
                }
            }
            return index_livre;
        }

        int firstInodeBlockIndex()
        {
             // Certifique-se de que a posição de leitura está correta
            this->fsFile.seekg(3 + bitmapSize);

            int index_livre = 0;
            INODE inode{};

            while (1)
            {
                this->fsFile.read((char *)&inode, sizeof(INODE));

                if (inode.IS_USED == 0)
                {
                    break;
                }
                index_livre++;
            }
            return index_livre;
        }

        void writeInode(int index, INODE inode)
        {
            this->fsFile.seekp(3 + this->bitmapSize + index * sizeof(INODE));
            this->fsFile.write((char *)&inode, sizeof(INODE));
        }

        void saveBitMap()
        {
            this->fsFile.seekp(3);
            this->fsFile.write(bitmap.get(), this->bitmapSize);
        }

        void setBitMap(int index)
        {
            getBitMap();
            int byteIndex = index/8;
            int bitOffSet = index % 8;

            bitmap[byteIndex] |= (1 << bitOffSet);

            saveBitMap();
        }

        void freeBitMap(int index)
        {
            getBitMap();
            uint8_t mask = 0xFF;             // todos os bits são 1
            mask ^= (1 << (index % 8));      // inverte o bit desejado para 0
            this->bitmap[index / 8] &= mask; // zera o bit desejado
            saveBitMap();
        }

        void writeDataBlock(int index, const char *data)
        {
            this->fsFile.seekp(3 + this->bitmapSize + this->indexVectorSize + 1 + index * this->blockSize);
            this->fsFile.write(data, this->blockSize);

        }

        char *readDataBlock(int index)
        {
            this->fsFile.seekg(3 + this->bitmapSize + this->indexVectorSize + 1 + index * this->blockSize);
            this->fsFile.read(tmpDataBlock.get(), this->blockSize);
            return tmpDataBlock.get();
        }

        string getFatherDirName(const std::string& file_path)
        {
            string father = file_path.substr(0, file_path.find_last_of("/")) == "" ? "/" : file_path.substr(0, file_path.find_last_of("/"));
            if (father != "/")
        {
            father = father.substr(1, father.size());
        }
            return father;
        }
        INODE appendDataIntoInode(INODE inode, char data)
        {
            // roda os direct block até achar um vazio e escreve o data
            inode.DIRECT_BLOCKS[(int)ceil((inode.SIZE + 1) / 3)] = data;
            return inode;
        }

        INODE insertDataInodeBlock(INODE inode, char data)
        {
            // verifica se a quantidade atual + 1 precisa alocar mais um bloco
            int oldBlockCount = static_cast<int>(std::ceil(static_cast<float>(inode.SIZE) / blockSize));
            int newBlockCount = static_cast<int>(std::ceil(static_cast<float>(inode.SIZE + 1) / blockSize));
            
            // Se é necessário alocar um novo bloco, faz a alocação
            if (newBlockCount > oldBlockCount && !(oldBlockCount == 0 && inode.IS_DIR == 1))
            {
                int freeBlockIndex = firstFreeBlockBitMap();    // Encontra um bloco livre
                setBitMap(freeBlockIndex);                      // Marca o bloco como ocupado
                inode = appendDataIntoInode(inode, static_cast<char>(freeBlockIndex));  // Adiciona o bloco ao `INODE`
            }
            // pega o endereço do ultimo bloco
            int lastBlockIndex = inode.DIRECT_BLOCKS[newBlockCount - 1];
            int positionInBlock = inode.SIZE % blockSize;

            writeByteBlock(lastBlockIndex, positionInBlock, data);
            
            inode.SIZE++;
            return inode;
        }

        INODE getInodeAtIndex(int index)
        {
            INODE inode{};
            this->fsFile.seekg(3 + this->bitmapSize + index * sizeof(INODE));
            this->fsFile.read((char *)&inode, sizeof(INODE));
            return inode;
        }

        INODE getInodeByName(std::string& name)
        {
            INODE inode = getInodeAtIndex(getInodeIndex(name));
            return inode;
        }

        int getInodeIndex(std::string& name)
        {
            // cout << "buscando inode com nome: " << name << endl;
            fsFile.seekg(3 + bitmapSize);

            INODE inode{};
            int position = 0;
            bool found = false;
            while (1)
            {
                this->fsFile.read((char *)&inode, sizeof(INODE));
                if (inode.IS_USED == 1)
                {
                    if (inode.NAME == name)
                    {
                        found = true;
                        break;
                    }
                }
                position++;
            }
            if (!found)
            {
                exit(1);
            }
            return position;
        }

        void createDir(const std::string& path)
        {
            // Inicializa o inode para o novo diretório
            INODE inode = createDirInode(path);

            // Encontra o primeiro inode e bloco de dados livres
            int freeInodeIndex = firstInodeBlockIndex();
            if (freeInodeIndex == -1) {
                throw std::runtime_error("No free inode available to create directory.");
            }

            int freeBlockIndex = firstFreeBlockBitMap();
            if (freeBlockIndex == -1) {
                throw std::runtime_error("No free data block available to allocate directory data.");
            }

            // Associa o bloco livre ao bloco direto do inode
            inode.DIRECT_BLOCKS[0] = static_cast<char>(freeBlockIndex);

            // Marca o bloco como utilizado no bitmap
            setBitMap(freeBlockIndex);

            // Escreve o inode no índice de inodes livres
            writeInode(freeInodeIndex, inode);
        }

        void createFile(string file_name)
        {
            // cria o inode do arquivo
            INODE file_inode = this->createFileInode(file_name);
            // descobre o index do primeiro bloco livre
            int index_livre = this->firstInodeBlockIndex();
            // escreve o inode no arquivo
            this->writeInode(index_livre, file_inode);
        }

        void addFileToDir(string path, string file_name)
        {
            // descobre o index do inode do pai no index vector
            int father_dir_inode_index = this->getInodeIndex(path);
            // cout << "father_dir_inode_index: " << father_dir_inode_index << endl;
            INODE father_dir_inode = this->getInodeAtIndex(father_dir_inode_index);

            // descobre o index do inode do arquivo no index vector
            int file_inode_index = this->getInodeIndex(file_name);
            // cout << "file_inode_index: " << file_inode_index << endl;
            INODE file_inode = this->getInodeAtIndex(file_inode_index);

            // adiciona o inode do arquivo no inode do pai
            // father_dir_inode.SIZE++;
            // cout << "adicionando o endereço do inode do arquivo no inode do pai" << endl;
            father_dir_inode = this->insertDataInodeBlock(father_dir_inode, (char)file_inode_index);
            // cout << "Endereço adicionado" << endl;

            // escreve o inode do pai no arquivo
            this->writeInode(father_dir_inode_index, father_dir_inode);
            // cout << "inode salvo" << endl;
        }

        void writeFile(string file_name, string content)
        {
            // descobre o index do inode do arquivo no index vector
            int file_inode_index = getInodeIndex(file_name);
            INODE file_inode = getInodeAtIndex(file_inode_index);

            // cout << endl
            //      << "adicionando o conteudo do arquivo no inode do arquivo" << endl;
            for (int i = 0; i < content.size(); i++)
            {
                file_inode = this->insertDataInodeBlock(file_inode, content[i]);
            }

            // escreve o inode do arquivo no arquivo
            this->writeInode(file_inode_index, file_inode);
        }

        void writeBlockAtIndex(int index, char *data)
        {
            this->fsFile.seekp(3 + this->bitmapSize + this->indexVectorSize + 1 + index * this->blockSize);
            this->fsFile.write(data, this->blockSize);
        }

        void writeByteBlock(int index, int position, char data)
        {
            char *data_block = readDataBlock(index);
            data_block[position] = data;
            writeDataBlock(index, data_block);
        }

        void defragmentDir(int index_dir_inode, INODE &dir_inode)
        {
            vector<char> dir_content; // vetor que vai armazenar os dados e depois vai reorganizar em blocos novamente
            for (int i = 0; i < 3; i++)
            {
                char *data_block = readDataBlock((int)dir_inode.DIRECT_BLOCKS[i]);
                // para cada byte do bloco verifica se o inode correspondente está vazio (IS_USED == 0)
                for (int j = 0; j < blockSize; j++)
                {
                    int inode_index = (int)data_block[j];
                    // pega o inode correspondente
                    INODE inode = getInodeAtIndex(inode_index);

                    bool found = false;
                    for (int k = 0; k < dir_content.size(); k++)
                    {
                        if (dir_content[k] == inode_index)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        dir_content.push_back(inode_index);
                    }
                }
            }

            // verifica a quantidade blocos necessários para armazenar o conteudo do dir
            int qtd_necessaria = ceil((float)dir_content.size() / (float)blockSize);
            // verifica a quantidade de blocos que o dir esta usando agora
            int qtd_atual = ceil((float)dir_inode.SIZE / (float)blockSize);

            // se a quantidade de blocos necessários for menor que a quantidade de blocos que o dir esta usando agora, libera os blocos que não serão mais usados no bitmap
            if (qtd_necessaria < qtd_atual)
            {
                for (int i = qtd_necessaria; i < qtd_atual; i++)
                {
                    this->freeBitMap((int)dir_inode.DIRECT_BLOCKS[i]);
                }
            }
            if (qtd_atual == 0)
            {
                return;
            }
            // remove do vetor os inodes que não estao mais sendo usados (IS_USED = 0)
            vector<char> dir_content_aux;
            for (int i = 0; i < dir_content.size(); i++)
            {
                INODE inode = getInodeAtIndex((int)dir_content[i]);
                if (inode.IS_USED == 1)
                {
                    dir_content_aux.push_back(dir_content[i]);
                }
            }
            dir_content = dir_content_aux;
            writeByteVectorInBlocks(dir_content, dir_inode);
        }

        void writeByteVectorInBlocks(vector<char> byte_vector, INODE inode)
        {
            // coloca os dados do vetor nos blocos que ele tem disponivel agora
            for (int i = 0; i < byte_vector.size(); i++)
            {
                writeByteBlock((int)inode.DIRECT_BLOCKS[(int)(i / 3)], i % this->blockSize, byte_vector[i]);
            }
        }

        std::vector<char> getBytesFromInodeBlocks(const INODE& inode)
        {
            std::vector<char> bytes;
            int totalBytes = inode.SIZE; // Tamanho real dos dados no inode
            int totalBlocks = std::ceil(static_cast<float>(totalBytes) / blockSize);

            for (int i = 0; i < totalBlocks; ++i) {
                if (inode.DIRECT_BLOCKS[i] == 0) {
                    break; // Bloco vazio, interrompe a leitura
                }

                // Lê o bloco usando ponteiro bruto
                char* rawBlock = readDataBlock(static_cast<int>(inode.DIRECT_BLOCKS[i]));

                std::unique_ptr<char[]> dataBlock(rawBlock);

                // Calcula os bytes a copiar deste bloco
                int bytesToCopy = std::min(blockSize, totalBytes - static_cast<int>(bytes.size()));
                bytes.insert(bytes.end(), dataBlock.get(), dataBlock.get() + bytesToCopy);
            }

            return bytes;
        }


        std::string getFileContent(const INODE& fileInode)
        {
            std::string content; // Inicializa a string para armazenar o conteúdo do arquivo
            int bytesRemaining = fileInode.SIZE; // Tamanho total do arquivo em bytes

            // Itera pelos blocos diretos do inode
            for (int i = 0; i < 3 && bytesRemaining > 0; ++i) {
                if (fileInode.DIRECT_BLOCKS[i] == 0) {
                    continue; // Pula blocos não alocados
                }

                // Lê o bloco de dados do disco
                char* rawBlock = readDataBlock(static_cast<int>(fileInode.DIRECT_BLOCKS[i]));

                // Encapsula o ponteiro bruto em um std::unique_ptr para gerenciamento automático
                std::unique_ptr<char[]> dataBlock(rawBlock);

                // Calcula o número de bytes a serem lidos deste bloco
                int bytesToRead = std::min(blockSize, bytesRemaining);

                // Adiciona os bytes ao conteúdo do arquivo
                content.append(dataBlock.get(), bytesToRead);

                // Atualiza os bytes restantes
                bytesRemaining -= bytesToRead;
            }

            return content; // Retorna o conteúdo completo
        }


        void removeFileRefFromDir(string path, int file_inode_index)
        {
            // descobre o index do inode do pai no index vector
            int father_dir_inode_index = this->getInodeIndex(path);
            INODE father_dir_inode = this->getInodeAtIndex(father_dir_inode_index);

            INODE file_inode = this->getInodeAtIndex(file_inode_index);

            vector<char> dir_content = this->getBytesFromInodeBlocks(father_dir_inode);

            // remove o byte cujo o valor é o index do inode do arquivo
            vector<char> tmp_char_vector;
            for (int i = 0; i < dir_content.size(); i++)
            {
                if (dir_content[i] != file_inode_index)
                {
                    tmp_char_vector.push_back(dir_content[i]);
                }
            }
            dir_content = tmp_char_vector;
            int antiga_qtd_necessaria = ceil((float)father_dir_inode.SIZE / (float)this->blockSize);

            father_dir_inode.SIZE--;

            // recalcula a quantidade de blocos necessários para armazenar o conteudo do dir
            int nova_qtd_necessaria = ceil((float)dir_content.size() / (float)this->blockSize);

            // cout << "A nova quantidade necessária é: " << nova_qtd_necessaria << endl;

            // verifica se houve mudança na qtd de blocos necessários
            if ((nova_qtd_necessaria != antiga_qtd_necessaria) && (nova_qtd_necessaria != 0))
            {                
                // se houve, libera os blocos que não serão mais usados no bitmap
                for (int i = nova_qtd_necessaria; i < antiga_qtd_necessaria; i++)
                {
                    // cout << "Liberando no bitmap o bloco: " << (int)father_dir_inode.DIRECT_BLOCKS[i] << endl;
                    freeBitMap((int)father_dir_inode.DIRECT_BLOCKS[i]);
                    father_dir_inode.DIRECT_BLOCKS[i] = 0;
                }
            }

            // escreve o inode do pai no arquivo
            writeInode(father_dir_inode_index, father_dir_inode);

            // reescreve o dir
            writeByteVectorInBlocks(dir_content, father_dir_inode);
        }

};

#endif