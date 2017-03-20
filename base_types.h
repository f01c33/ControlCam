/// cada request pode ser tratado como nó em uma árvore, porém utilizando o nome
/// e uma tabela hash de todos os requests para acessar os comandos, não testei
/// a velocidade comparando com ponteiros pra structs como normalmente, mas pelo
/// menos não é necessário alocar todos os comandos quando executando um request
/// Essencialmente a versão em C dos JSONs encontrados na pasta config
typedef struct request {
  double fact_to_next; // Fator de multiplicação do tempo para executar o
                       // próximo ou o                              // anterior
                       // comando, passado ou não pelo argumento da função
  fstr name;           // Nome no request
  fstr next_cmd;       // Nome do proximo comando, pode ser NULL
  fstr prev_cmd;       // Nome do comando anterior, pode ser NULL
  fstr base;           // Request em si, (o que será enviado a câmera)
  ffstr headers;       // Vetor de headers, pode ser NULL
  ffstr args;          // Vetor de argumentos, passado
} request;

// vector<request>
MAKEFAT(request);
FCMP(request, a, b) { return CMP(fstr)(a.name, b.name); }

/// Cada tipo de câmera terá suas configurações nesta struct, que contém nome,
/// headers gerais para todos os requests, e todos os requests possíveis, como
/// descrito em seus respectivos arquivos .JSON
typedef struct cam_struct {
  fstr name;
  ffstr headers;
  frequest requests;
} cam_cfg;

/// keyval é uma soma de duas strings, pode ser utilizada para construir-se uma
/// tabela hash,
/// mas foi utilizada em um vetor ordenado somente, na forma de fkeyval
typedef struct {
  fstr key;
  fstr val;
} keyval;

MAKEFAT(keyval)
FCMP(keyval, a, b) { return CMP(fstr)(a.key, b.key); }
