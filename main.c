#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

// #include <dirent.h>                  // diretórios

#include "./deps/b64/b64.h"             // fazer encode do base64
#include "./deps/fat_array/fat_array.h" // array dinâmica
#include "./deps/flag/flag.h"           // lidar com argv e argc
#include "./deps/jsmn/jsmn.h"           // json parser lib

#include <curl/curl.h> // para comunicação web no geral

#define VERSION "0.0.1" // pré-git

// missing definitions from time.h /*
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 199309L

struct timespec {
  time_t tv_sec;
  long tv_nsec;
};

int nanosleep(const struct timespec *req, struct timespec *rem);
#endif
// */

// pode parecer confuso definir str como char, mas como vou utilizar fchar como
// uma string,
// faz mais sentido chamar char de str, assim fchar vira fstr
typedef char str;

MAKEFAT(str);
FCMP(str, a, b) { return a - b; }

/// É a função strstr só que funcionando ao contrário, procura a string needle
/// do final ao começo.
size_t revstrstr(const char *haystack, const char *needle,
                 const size_t haystacklen) {
  static const char *current;
  static size_t left;
  if (haystack != NULL) {
    current = haystack;
    left = haystacklen - 1;
  }
  size_t needle_len = strlen(needle);
  for (int j; left >= needle_len; left--) {
    // printf("%c == %c\n",current[left],needle[needle_len-1]);
    for (j = 0;
         current[left - j] == needle[needle_len - 1 - j] && j < needle_len;
         j++) {
      // printf("j:%d\n",j);
    }
    if (j == needle_len) {
      left--;
      return left + 1;
    }
  }
  return -1; // will explode if tried to assign on
}

/// fstr_replace substitui todas as ocorrencias de needle por substitute, para
/// isso utilizamos
/// da strstr para contar o numero de ocorrencia de needle, então utiliza deste
/// valor, tal
/// como o tamanho das strings que serão a base para a mudança de tamanho, caso
/// necessária.
/// utilizando o revstrstr é possível copiar com um passo na string base
/// utilizando memcpy,
/// que segundo o padrão ISO C 99 pode ser tratado como built-in, podendo
/// utilizar a instrução
/// ASM adequada, tornando o programa um pouco mais rápido.
/// O(2N) (talvez, ainda não tive isso em aula)
fstr fstr_replace(fstr base, const char *needle, const char *substitute) {
  int cont_needle = 0;
  for (char *tmp = strstr(base, needle);
       tmp != NULL; /// counts the number of occurences of needle
       tmp = strstr(tmp + 1, needle)) {
    // printf("%s\n",tmp);fflush(stdout);
    cont_needle += 1;
  }
  if (cont_needle == 0) {
    return base;
  }
  int needle_len = strlen(needle);
  int substitute_len = strlen(substitute);
  // ensure we have space
  base = fat_growzero(str, base, (substitute_len - needle_len) * cont_needle);
  // copy things
  int k = (substitute_len - needle_len) * cont_needle;
  int next = revstrstr(base, needle, fat_len(str, base));
  // printf("needle: %i,subst: %i\n",needle_len,substitute_len);
  // printf("k:%d, next: %d\n",k,next);
  for (int i = fat_len(str, base) + k; i >= 0; i--) {
    // printf("%d; base[i] = %c, base[i-k] =
    // %c\n",i,base[i],base[i-k]);fflush(stdout);
    base[i] = base[i - k];
    if (i - k == next) {
      //   printf("WOO!");fflush(stdout);
      i -= substitute_len - 1;
      k -= (substitute_len - needle_len);
      if (i < 0) {
        i = 0;
      }
      memcpy(&base[i], substitute, substitute_len);
      next = revstrstr(NULL, needle, 0);
      // printf("k:%d, next: %d, i%d\n",k,next,i);fflush(stdout);
    }
  }
  return base;
}

/// char**, com um pouco de açucar (vide fat_array.h), ou vector<string> com
/// macros
MAKEFAT(fstr);
FCMP(fstr, a, b) { return strcmp(a, b); }

// input must be null terminated, otherwise undefined behaviour shall occur,
// it's also destructive to the input string's dividers

// can be used stand-alone with this struct
// struct explode_out {
//   size_t len;
//   char *stg[];
// }; // this is real nice
// struct explode_out explode(char* input, const char* divider){
/// Divide a fstr input todas as ocorrencias dos chars divider e retorna um
/// vetor de strings, ou ffstr.
ffstr fstr_explode(char *input, const char divider[]) {
  // char *input = &input[0];
  char *divtmp = &input[0];
  // size_t lendiv = strlen(divider);
  // size_t cont = 1;
  ffstr out = fat_new(fstr, 8);
  fat_push(fstr, out, input);
  while (input[0] != '\0') { // count the number of dividers inside the string
    divtmp = divider;
    while (divtmp[0] != '\0') {
      if (input[0] == divtmp[0]) {
        input[0] = '\0';
        out = fat_push(fstr, out, input + 1);
      }
      divtmp++;
    }
    input++;
  }
  return out;
}

fstr ffstr_join(char** input,int len,char sep){
  int total_size = 0;
  for(int i = 0; i < len; i++){
    total_size += strlen(input[i]);
  }
  fstr out = fat_new(str,total_size+len+1); // separator is not put on the end
  for(int i = 0; i < len; i++){
    for(int j = 0; input[i][j] != 0;j++){
      out = fat_push(str,out,input[i][j]);
    }
    out = fat_push(str,out,sep);
  }
  out[fat_len(str,out)-1]='\0';
  // out = fat_push(str,out,'\0');
  return out;
}

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

// frees everything.
void request_free(request rqst) {
  fat_free(str, rqst.name);
  fat_free(str, rqst.base);
  fat_free(str, rqst.next_cmd);
  fat_free(str, rqst.prev_cmd);

  for (int j = 0; rqst.headers != NULL && j < fat_len(fstr, rqst.headers);
       j++) {
    fat_free(str, rqst.headers[j]);
  }
  for (int j = 0; rqst.args != NULL && j < fat_len(fstr, rqst.args); j++) {
    fat_free(str, rqst.args[j]);
  }
  //  printf("len: %zu",fat_len(str,rqst.args));fflush(stdout);
}

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

void cam_cfg_free(cam_cfg *cfg) {
  if (cfg == NULL) {
    return;
  }
  fat_free(str, cfg->name);   // there's a null check at free
  if (cfg->headers != NULL) { // but not at len
    for (int i = 0; i < (int)fat_len(fstr, cfg->headers); i++) {
      fat_free(str, cfg->headers[i]);
    }
    fat_free(fstr, cfg->headers);
  }
  if (cfg->requests != NULL) {
    for (int i = 0; i < (int)fat_len(request, cfg->requests); i++) {
      request_free(cfg->requests[i]);
    }
  }
  free(cfg);
}

// modified from libcurl example
// reaaaally got to refactor this function, it should recieve a complete (worked
// up) url, and a header f_array
int curl_rqst(const char *auth, const char *url, const char *base,
              const char *arg, const char *filename) {
  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  struct curl_slist *list = NULL;
  FILE *log = fopen(filename, "a");
  if (log == NULL) {
    fprintf(stderr, "Unable to open file %s, printing to stdout\n", filename);
    log = stdout;
  }
  char curl_url[1 << 7];
  char curl_header[1 << 6];

  if (curl) {
    strcpy(&curl_url[0], "http://");
    strcat(&curl_url[0], url);
    strcat(&curl_url[0], base);
    strcat(&curl_url[0], arg);

    strcpy(&curl_header[0], "Authorization: Basic ");
    strcat(&curl_header[0], auth);

    curl_easy_setopt(curl, CURLOPT_URL, curl_url);
    // printf("%s,%s\n",curl_url,filename);
    /* example.com is redirected, so we tell libcurl to follow redirection */
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)log);

    list = curl_slist_append(list, curl_header);
    /* Perform the request, res will get the return code */

    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, list);

    res = curl_easy_perform(curl);
    /* Check for errors */
    if (res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
      // return -1;
      goto curl_rqst_clean;
    }
  curl_rqst_clean:
    /* always cleanup */
    curl_easy_cleanup(curl);
    curl_slist_free_all(list);
  }
  fclose(log);
  return 0;
}

// void list_print() {
//   unsigned short int apexis_len = sizeof(apexis) / sizeof(apexis[0]);
//   for (int i = 0; i < apexis_len; i++) {
//     printf("\t%s, %d\n", apexis[i].idx, apexis[i].len);
//   }
// }

// divides sleep automatically, whole part goes to seconds, fractional part goes
// to nanoseconds, as tv_nsec can only hold a value so big
void n_sleep(double sec) {
  static double integer;
  static struct timespec wt[2] = {{0, 0}, {0, 0}};
  // printf("%f\n",sec);
  wt[0].tv_nsec = modf(sec, &integer) * 1000000000L;
  wt[0].tv_sec = integer;
  nanosleep(wt, NULL);
}

/// lê o arquivo filename em uma fstr, caso stg seja NULL, retornará uma nova
/// fstr (que deve
/// ser liberada)
fstr file_to_fstr(fstr stg, const char *filename) {
  FILE *file = fopen(filename, "r");
  if (file == NULL) {
    fprintf(stderr, "FILE %s does not exist!\n", filename);
    return NULL;
  }
  fstr out = NULL;
  if (stg == NULL) {
    out = fat_new(str, 128);
  } else {
    out = stg;
  }
  // int i = 0;
  while (!feof(file)) {
    out = fat_push(str, out, fgetc(file));
    // printf("%c",out[i]);
    // i++;
  }
  fclose(file);
  out[fat_len(str, out) - 1] = '\0'; // EOF = '\0'
  return out;
}

/// Verifica se o campo json do token tok é igual a s
static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

/// keyval é uma soma de duas strings, pode ser utilizada para construir-se uma
/// tabela hash,
/// mas foi utilizada em um vetor ordenado somente, na forma de fkeyval
typedef struct {
  fstr key;
  fstr val;
} keyval;

MAKEFAT(keyval)
FCMP(keyval, a, b) { return CMP(fstr)(a.key, b.key); }

/// copia um jsmntok para uma fstr separada.
static fstr fstr_from_jstok(fstr base, jsmntok_t tok) {
  if (tok.type != JSMN_STRING) {
    return NULL;
  }
  fstr out = fat_new(str, tok.end - tok.start + 1);
  for (int i = 0; i <= tok.end - tok.start; i++) {
    out = fat_push(str, out, base[tok.start + i]);
  }
  out[fat_len(str, out) - 1] = '\0';
  return out;
}

/// A grande função que lê a configuração da câmera de nome config,
fkeyval read_camera_args(const char *config) {
  char cam_name[1 << 7];
  strcpy(&cam_name[0], "./cameras/");
  strcat(&cam_name[0], config);
  strcat(&cam_name[0], ".json");
  fstr cam_cfg_txt = file_to_fstr(NULL, cam_name);

  fkeyval out;

  keyval tmpkv;

  out = fat_new(keyval, 4);

  if (cam_cfg_txt == NULL) {
    return NULL;
  }

  jsmn_parser jsparser;
  jsmn_init(&jsparser);

  int tokens =
      jsmn_parse(&jsparser, cam_cfg_txt, fstr_len(cam_cfg_txt), NULL, 0);
  jsmntok_t jstokens[tokens];

  jsmn_init(&jsparser);
  tokens = jsmn_parse(&jsparser, cam_cfg_txt, fstr_len(cam_cfg_txt), jstokens,
                      tokens);
  if (tokens < 0) {
    fprintf(stderr, "tokens = %d at file %s on line %i\n", tokens, config,
            __LINE__);
  }
  for (int i = 0; i < tokens; i++) {
    if (jstokens[i].type == JSMN_STRING && jstokens[i].size > 0) {
      tmpkv.key = fstr_from_jstok(cam_cfg_txt, jstokens[i]);
      i++;
      tmpkv.val = fstr_from_jstok(cam_cfg_txt, jstokens[i]);
      out = fat_push(keyval, out, tmpkv);
    }
  }
  fat_sort(keyval, out, CMP(keyval));
  fat_free(str, cam_cfg_txt);
  return out;
}

void jstok_print(fstr base, jsmntok_t tok) {
  for (int i = 0; i < tok.end - tok.start; i++) {
    printf("%c", base[tok.start + i]);
  }
}

/// lê o tipo da câmera da pasta /config/cam_type.json
cam_cfg *read_cam_config(const char *cam_type) {
  char tmp[1 << 7];
  strcpy(tmp, "./config/");
  strcat(tmp, cam_type);
  strcat(tmp, ".json");

  fstr cam_cfg_txt = file_to_fstr(NULL, tmp);
  if (cam_cfg_txt == NULL) {
    return NULL;
  }

  jsmn_parser jsparser;
  jsmn_init(&jsparser);

  int tokens =
      jsmn_parse(&jsparser, cam_cfg_txt, fstr_len(cam_cfg_txt), NULL, 0);
  jsmntok_t jstokens[tokens];

  // printf("tokens: %i\n", tokens);

  jsmn_init(&jsparser);
  tokens = jsmn_parse(&jsparser, cam_cfg_txt, fstr_len(cam_cfg_txt), jstokens,
                      tokens);

  cam_cfg *out = malloc(sizeof(cam_cfg));
  out->headers = NULL;
  out->requests = NULL;
  out->name = NULL;

  fstr tmpstr = NULL;
  request rqst = (request){0};

  bool is_cmd = false;

  for (int i = 0; i < tokens; i++) {
    // printf("type: %i, start: %i, end: %i, size: %i\n", jstokens[i].type,
    //        jstokens[i].start, jstokens[i].end, jstokens[i].size);
    if (0 == jsoneq(cam_cfg_txt, &jstokens[i], "name")) {
      i++;
      if (jstokens[i].type != JSMN_STRING) {
        fprintf(stderr,
                "Invalid name in file %s at line: %d, it should be a string\n",
                cam_type, __LINE__);
        goto cleanup_error0;
      }
      out->name = fstr_from_jstok(cam_cfg_txt, jstokens[i]);
    } else if (!is_cmd && 0 == jsoneq(cam_cfg_txt, &jstokens[i], "headers")) {
      // do headers parsing, remember size!, and null
      i++;
      // printf("HDR: type: %i, start: %i, end: %i, size: %i\n",
      // jstokens[i].type,
      //        jstokens[i].start, jstokens[i].end, jstokens[i].size);
      if (jstokens[i].size == 0 && jstokens[i].type != JSMN_STRING) {
        if (cam_cfg_txt[jstokens[i].start] == 'n') {
          out->headers = NULL;
        } else {
          fprintf(stderr,
                  "Invalid header in file %s at line %d, it should be a null "
                  "or a string array\n",
                  cam_type, __LINE__);
        }
      } else {
        int size = jstokens[i].size;
        out->headers = fat_new(fstr, size);
        //        i++;
        for (int j = 0; j < size; j++) { // move to next element
          i++;
          tmpstr = fstr_from_jstok(cam_cfg_txt, jstokens[i]);
          out->headers = fat_push(fstr, out->headers, tmpstr);
          tmpstr = NULL;
        }
      }
    } else if (0 == jsoneq(cam_cfg_txt, &jstokens[i], "commands")) {
      // do commands parsing, remember size!, and null
      // printf("COMMAND: type: %i, start: %i, end: %i, size: %i\n",
      //        jstokens[i].type, jstokens[i].start, jstokens[i].end,
      //        jstokens[i].size);
      is_cmd = true;
      i++;
      int size = jstokens[i].size; // array length
      // printf("cmd_size = %d\n", size);
      if (size == 0 && jstokens[i].type != JSMN_ARRAY) {
        out->requests = NULL;
        fprintf(stderr, "No valid commands found in file %s at line %d\n",
                cam_type, __LINE__);
        goto cleanup_error1;
      }
      out->requests = fat_new(request, size);
      for (int j = 0; j < size; j++) {
        // printf("NEW_RQST\n");
        rqst.name = NULL;
        rqst.base = NULL;
        rqst.next_cmd = NULL;
        rqst.prev_cmd = NULL;
        rqst.headers = NULL;
        rqst.args = NULL;
        // printf("rqst_array: type: %i, start: %i, end: %i, size: %i\n",
        //        jstokens[i].type, jstokens[i].start, jstokens[i].end,
        //        jstokens[i].size);
        i++;
        if (jstokens[i].type != JSMN_OBJECT) {
          fprintf(stderr,
                  "commands should have an array of objects (requests), it "
                  "does not, currently, it has type %d; file %s at line %i\n",
                  jstokens[i].type, cam_type, __LINE__);
          goto cleanup_error2;
        }
        int rqst_size = jstokens[i].size;
        // printf("js_cmd: ");
        // jstok_print(cam_cfg_txt, jstokens[i]);
        // puts("");
        // printf("rqst_size = %d\n",rqst_size);
        for (int k = 0; k < rqst_size; k++) { // parsing each request
          i++;
          if (0 == jsoneq(cam_cfg_txt, &jstokens[i], "name")) {
            // printf("cmd_name: type: %i, start: %i, end: %i, size: %i\n",
            //        jstokens[i].type, jstokens[i].start, jstokens[i].end,
            //        jstokens[i].size);
            i++;
            if (jstokens[i].type != JSMN_STRING) {
              fprintf(stderr, "Name should be a string in file %s at line %i\n",
                      cam_type, __LINE__);
              goto cleanup_error2;
            }
            rqst.name = fstr_from_jstok(cam_cfg_txt, jstokens[i]);
            // tmpstr = NULL;
            // printf("cmd_name = %s\n",rqst.name);
          } else if (0 == jsoneq(cam_cfg_txt, &jstokens[i], "base")) {
            // printf("cmd_base: type: %i, start: %i, end: %i, size: %i\n",
            //        jstokens[i].type, jstokens[i].start, jstokens[i].end,
            //        jstokens[i].size);
            i++;
            if (jstokens[i].type != JSMN_STRING) {
              fprintf(stderr, "Base should be a string in file %s at line %d\n",
                      cam_type, __LINE__);
              goto cleanup_error2;
            }
            rqst.base = fstr_from_jstok(cam_cfg_txt, jstokens[i]);
            // printf("cmd_base = %s\n",rqst.base);
          } else if (0 == jsoneq(cam_cfg_txt, &jstokens[i], "fact_to_next")) {
            // printf("cmd_fact_to_next: type: %i, start: %i, end: %i, size:
            // %i\n",
            //        jstokens[i].type, jstokens[i].start, jstokens[i].end,
            //        jstokens[i].size);
            // test if null
            i++;
            if (jstokens[i].type == JSMN_PRIMITIVE) {
              if (cam_cfg_txt[jstokens[i].start] == 'n') {
                rqst.fact_to_next = 0.0;
              } else { // should also test if it's not a number
                tmpstr = fstr_from_jstok(cam_cfg_txt, jstokens[i]);
                rqst.fact_to_next = strtod(tmpstr, NULL);
                fat_free(str, tmpstr);
              }
            }
          } else if (0 == jsoneq(cam_cfg_txt, &jstokens[i], "next_cmd")) {
            // printf("cmd_next_cmd: type: %i, start: %i, end: %i, size: %i\n",
            //        jstokens[i].type, jstokens[i].start, jstokens[i].end,
            //        jstokens[i].size);
            i++;
            if (jstokens[i].type == JSMN_PRIMITIVE) {
              if (cam_cfg_txt[jstokens[i].start == 'n']) {
                rqst.next_cmd = NULL;
              } else {
                fprintf(stderr,
                        "next_cmd should be either null, or the name of the "
                        "next command, in file %s, at line %i\n",
                        cam_type, __LINE__);
              }
            } else if (jstokens[i].type == JSMN_STRING) {
              rqst.next_cmd = fstr_from_jstok(cam_cfg_txt, jstokens[i]);
              if (rqst.next_cmd == NULL) {
                fprintf(stderr,
                        "next_cmd should be either null, or the name of the "
                        "next command, in file %s, at line %i\n",
                        cam_type, __LINE__);
              }
            }
          } else if (0 == jsoneq(cam_cfg_txt, &jstokens[i], "prev_cmd")) {
            // printf("cmd_next_cmd: type: %i, start: %i, end: %i, size: %i\n",
            //        jstokens[i].type, jstokens[i].start, jstokens[i].end,
            //        jstokens[i].size);
            i++;
            if (jstokens[i].type == JSMN_PRIMITIVE) {
              if (cam_cfg_txt[jstokens[i].start == 'n']) {
                rqst.prev_cmd = NULL;
              } else {
                fprintf(stderr,
                        "prev_cmd should be either null, or the name of the "
                        "next command, in file %s, at line %i\n",
                        cam_type, __LINE__);
              }
            } else if (jstokens[i].type == JSMN_STRING) {
              rqst.prev_cmd = fstr_from_jstok(cam_cfg_txt, jstokens[i]);
              if (rqst.prev_cmd == NULL) {
                fprintf(stderr,
                        "prev_cmd should be either null, or the name of the "
                        "next command, in file %s, at line %i\n",
                        cam_type, __LINE__);
              }
            }

          } else if (0 == jsoneq(cam_cfg_txt, &jstokens[i], "headers")) {
            // printf("cmd_hdr: type: %i, start: %i, end: %i, size: %i\n",
            //        jstokens[i].type, jstokens[i].start, jstokens[i].end,
            //        jstokens[i].size);
            // array, maybe
            i++;
            if (jstokens[i].type == JSMN_PRIMITIVE) {
              if (cam_cfg_txt[jstokens[i].start] == 'n') {
                rqst.headers = NULL;
              } else {
                fprintf(stderr,
                        "header should be either null, or an array containing "
                        "the headers as strings, in file %s, at line %i\n",
                        cam_type, __LINE__);
                goto cleanup_error3;
              }
            } else if (jstokens[i].type == JSMN_ARRAY) {
              int hdr_size = jstokens[i].size;
              rqst.headers = fat_new(fstr, hdr_size);
              for (int l = 0; l < hdr_size; l++) { // parse header array
                i++;
                tmpstr = fstr_from_jstok(cam_cfg_txt, jstokens[i]);
                if (tmpstr == NULL) {
                  fprintf(
                      stderr,
                      "header elements should be strings! file: %s, line: %i\n",
                      cam_type, __LINE__);
                  goto cleanup_error4;
                }
                rqst.headers = fat_push(fstr, rqst.headers, tmpstr);
              }
              tmpstr = NULL;
            } else {
              fprintf(stderr,
                      "header should be either null, or an array containing "
                      "the headers as strings, in file %s, at line %i\n",
                      cam_type, __LINE__);
            }
          } else if (0 == jsoneq(cam_cfg_txt, &jstokens[i], "args")) {
            // printf("cmd_args: type: %i, start: %i, end: %i, size: %i\n",
            //        jstokens[i].type, jstokens[i].start, jstokens[i].end,
            //        jstokens[i].size);
            // array, maybe
            i++;
            if (jstokens[i].type == JSMN_PRIMITIVE) {
              if (cam_cfg_txt[jstokens[i].start] == 'n') {
                rqst.args = NULL;
              } else {
                fprintf(stderr,
                        "args should be either null, or an array containing "
                        "the args as strings, in file %s, at line %i\n",
                        cam_type, __LINE__);
              }
            } else if (jstokens[i].type == JSMN_ARRAY) {
              int arg_size = jstokens[i].size;
              rqst.args = fat_new(fstr, arg_size);
              for (int l = 0; l < arg_size; l++) {
                i++;
                tmpstr = fstr_from_jstok(cam_cfg_txt, jstokens[i]);
                if (tmpstr == NULL) {
                  fprintf(
                      stderr,
                      "arg elements should be strings! file: %s, line: %i\n",
                      cam_type, __LINE__);
                  goto cleanup_error4;
                }
                // printf("args = %s",tmpstr);fflush(stdout);
                rqst.args = fat_push(fstr, (rqst.args), tmpstr);
              }
              tmpstr = NULL;
              i++;
            } else {
              fprintf(stderr,
                      "args should be either null, or an array containing the "
                      "args as strings, in file %s, at line %i\n",
                      cam_type, __LINE__);
            }
          } // new commands could go here, prehaps another field, maybe
            // adressable as a "macro"
          // else{
          //   printf("\nwat\n");
          //   jstok_print(cam_cfg_txt,jstokens[i]);
          // }
        }
        // printf("\nname: %s, base: %s, factor_to_next: %ld, next_cmd =
        // %d\nargs: ",rqst.name,rqst.base,rqst.fact_to_next,rqst.next_cmd);
        // for(int k = 0; k < fat_len(fstr,rqst.args); k++){
        //     printf("%s, ",rqst.args[k]);
        // }
        // printf("hdrs: ");
        // for(int k = 0; k < rqst.headers != NULL &&
        // fat_len(fstr,rqst.headers); k++){
        //     printf("%s, ", rqst.headers[k]);
        // }
        // puts("\nend_rqst");
        out->requests = fat_push(request, out->requests, rqst);
      }
    }
  }
  fat_free(str, cam_cfg_txt);
  fat_sort(request, out->requests, CMP(request));
  return out;

cleanup_error4:
  if (tmpstr != NULL) {
    fat_free(str, tmpstr);
  }
cleanup_error3:
cleanup_error2:
cleanup_error1:
  request_free(rqst);
cleanup_error0:
  cam_cfg_free(out);
  return NULL;
}

void print_cam_cfg(cam_cfg *cfg) {
  printf("name: %s\n", cfg->name);
  for (int i = 0; i < (int)fat_len(fstr, cfg->headers); i++) {
    printf("hdr %i: %s\n", i, cfg->headers[i]);
  }
  for (int i = 0; i < (int)fat_len(request, cfg->requests); i++) {
    printf("{name: %s\n", cfg->requests[i].name);
    if (cfg->requests[i].headers) {
      printf("hdr: ");
      for (int j = 0; j < fat_len(fstr, cfg->requests[i].headers); j++) {
        printf("\"%s\", ", cfg->requests[i].headers[j]);
      }
      printf("\n");
    } else {
      printf("hdr: (null)\n");
    }
    if (cfg->requests[i].args) {
      printf("args: ");
      for (int j = 0; j < fat_len(fstr, cfg->requests[i].args); j++) {
        printf("\"%s\", ", cfg->requests[i].args[j]);
      }
      printf("\n");
    } else {
      printf("args: (null)\n");
    }
    printf("base: %s\n", cfg->requests[i].base);
    printf("factor_to_next: %f\n", cfg->requests[i].fact_to_next);
    printf("prev_cmd: %s\n", cfg->requests[i].next_cmd);
    printf("next_cmd: %s\n", cfg->requests[i].next_cmd);
    printf("}\n");
  }
}
/// procura e subistutui todos os macros na string stg que estão no fkeyval
/// table
fstr substitute_macros(fstr stg, fkeyval table) {
  char tmp[1 << 7];
  for (int i = 0; i < fat_len(keyval, table); i++) {
    if (strcmp(table[i].key, "type") == 0 ||
        strcmp(table[i].key, "name") == 0 ||
        strcmp(table[i].key, "function") == 0) {
      continue;
    } else {
      strcpy(tmp, "{{");
      // printf("tkey: %s\n",table[i].key);
      strcat(tmp, table[i].key);
      strcat(tmp, "}}\0");
      stg = fstr_replace(stg, tmp, table[i].val);
    }
  }
  return stg;
}

struct command {
  fstr name;
  fstr arg;
  fstr factor;
};
// process the argument, subtitute macros, execute commands
// maybe use a recursive function to do it:
// maybe only the base parameter can have an argument
// or maybe one can use named arguments (fkeyval) as input parameters
//
// void execute_cmds(cam_cfg* cfg, fkeyval cam, ffstr arguments, int arg_idx,
// fstr current_cmd){
// //substitute macro on current_cmd as needed
// //call FUNC on the command
// return
//}
//
void run_cmd_tree(cam_cfg *cfg, fkeyval table, char* cmd_name) {
  // if (cmd_name == NULL) {
  //   return;
  // }
  request tmp = (request){.name = cmd_name};
  request command =
      cfg->requests[fat_bsearch(request, cfg->requests, CMP(request), tmp)];

  run_cmd_tree(cfg, table, command.prev_cmd); // to the left

  // ffstr headers = cfg->headers;
  ffstr headers = fat_new(fstr, fat_len(fstr, cfg->headers));
  for (int i = 0; headers != NULL && i < fat_len(fstr, headers); i++) {
    headers[i] = substitute_macros(fat_dup(str, cfg->headers[i]), table);
  }

  // duplicates the strings to that it does not change the underlying data
  command.base = substitute_macros(fat_dup(str, command.base), table);
  for (int i = 0; command.headers && i < fat_len(str, command.headers); i++) {
    command.headers[i] =
        substitute_macros(fat_dup(str, command.headers[i]), table);
  }
  printf("base_hdrs: \n");
  for (int i = 0; headers != NULL && i < fat_len(fstr, headers); i++) {
    printf("bhdr[%i]:%s\n", i, headers[i]);
  }
  printf("name: %s, base: %s, headers:\n", command.name, command.base);
  for (int i = 0; command.headers != NULL && i < fat_len(fstr, command.headers);
       i++) {
    printf("hdr[%i]:%s", i, command.headers[i]);
    // call correct function here
  }

  run_cmd_tree(cfg, table, command.next_cmd); // to the right

  fat_free(str, command.base);
  for (int i = 0; command.headers && i < fat_len(str, command.headers); i++) {
    fat_free(str, command.headers[i]);
  }
  for (int i = 0; headers != NULL && i < fat_len(fstr, headers); i++) {
    fat_free(str, headers[i]);
  }
  fat_free(fstr, headers);
  return;
}


int main(int argc, char *argv[]) {
  //ignoring program name, as it's not usefull in this case'
  fstr argjoin = ffstr_join(argv+1,argc-1,' ');
  puts(argjoin);

  ffstr arguments = fstr_explode(argjoin, " -");
  // printf("len:%zu,alloc:%zu\n", fat_len(fstr, arguments),
  //        fat_alloc(fstr, arguments));
  // for (int i = 0; i < fat_len(fstr, arguments); i++) {
  //   printf("%i:%s\n", i, arguments[i]);
  //   fflush(stdout);
  // }
  // for(int i = 0; i < fat_len(str,arguments);i++){
    // arguments[i] = fstr_replace(arguments[i]," ","");
    // puts(arguments[i]);
  // }
  fstr camera = arguments[0];

  int ret = 0;
  // fkeyval cam = read_camera_args("cam1");
  fkeyval cam = read_camera_args(camera);

  if (cam == NULL) {
    fprintf(stderr, "camera of name %s not found\n", camera);
    goto error_cam_args;
  }

  keyval tmp = {.key = "type", .val = NULL};

  // for (int i = 0; i < fat_len(keyval, cam); i++) {
  //   printf("%s\n", cam[i].key);
  // }

  int type_i = fat_bsearch(keyval, cam, CMP(keyval), tmp);

  cam_cfg *cfg = read_cam_config(cam[type_i].val);
  if (cfg == NULL) {
    fprintf(stderr,
            "Something went while parsing the camera config, aborting\n");
    goto error_cam_cfg;
  }
  // print_cam_cfg(cfg);
  fat_sort(request, cfg->requests, CMP(request)); // so we can find the
  // request with a binary search

  //run commands here
  // for(int i = 1; i < fat_len(str,arguments);i++){

    run_cmd_tree(cfg, cam, "up");
  // }
  // print_cam_cfg(cfg);

  if (0) {
  error_cam_cfg:
    ret = -1;
  }

  cam_cfg_free(cfg);

  for (int i = 0; i < (int)fat_len(keyval, cam); i++) {
    // printf("%s : %s\n", cam[i].key, cam[i].val);
    fat_free(str, cam[i].key);
    fat_free(str, cam[i].val);
  }
  fat_free(keyval, cam);
  if (0) {
  error_cam_args:
    ret = -2;
  }

  fat_free(fstr, arguments);
  return ret;
}

// int main(int argc, char const *argv[]) {
//   const char *user_pw = "utfpr:utfpr";
//   const char *url = "192.168.100.1:666";
//   const char *logging = "/dev/null";
//   bool help = false, version = false, list = false;
//   const char *angle = "0.0";

//   flagset_t *set = flagset_new();
//   flagset_string(set, &url, "u", "Url");
//   flagset_string(set, &user_pw, "l", "Login:password");
//   flagset_string(set, &angle, "a", "define angle of movement, when
//   applicable");
//   flagset_string(set, &logging, "log", "where to log (append) http
//   responses");
//   flagset_bool(set, &list, "list", "List Commands");
//   flagset_bool(set, &help, "help", "Output help");
//   flagset_bool(set, &version, "version", "Output version");
//   flagset_parse(set, argc, argv);

//   if (help) {
//     flagset_write_usage(set, stdout, argv[0]);
//     return 0;
//   } else if (version) {
//     printf(VERSION "\n");
//     return 0;
//   } else if (list) {
//     list_print();
//     return 0;
//   }
//   argv = set->argv;
//   argc = set->argc;
//   unsigned short int apexis_len = sizeof(apexis) / sizeof(apexis[0]);
//   char auth[1 << 5], base[1 << 5], arg[1 << 5];
//   int pos = 0; // acha posição do comando
//   if (argc == 1) {
//     flagset_write_usage(set, stdout, argv[0]);
//     return -1;
//   }
//   while (pos < apexis_len && strcmp(argv[1], apexis[pos].idx) != 0) {
//     pos++;
//   }
//   if (pos == apexis_len) {
//     puts("Invalid_command");
//     return -1;
//   }
//   if (argc == 2) {
//     strcpy(auth, b64_encode((unsigned char *)user_pw, strlen(user_pw)));
//     strcpy(base, apexis[pos].base);
//     strcpy(arg, apexis[pos].args[0]);
//   } else if (argc == 3) {
//     if (atoi(argv[3]) >= apexis[pos].len) {
//       printf("Invalid argument number, the maximum command %s has %d options,
//       "
//              "starting from 0",
//              argv[2], apexis[pos].len);
//       return -1;
//     }
//     strcpy(auth, b64_encode((unsigned char *)user_pw, strlen(user_pw)));
//     strcpy(base, apexis[pos].base);
//     strcpy(arg, apexis[pos].args[atoi(argv[3])]);
//   } else {
//     flagset_write_usage(set, stdout, argv[0]);
//     return -1;
//   }
//   if (apexis[pos].stop) {
//     if (angle[0] == '0' && angle[1] == '.' && angle[2] == '0') {
//       command(auth, url, base, arg, logging);
//       n_sleep(1.5);
//     } else {
//       command(auth, url, base, arg, logging);
//       n_sleep(atof(angle) * apexis[pos].sec_per_deg);
//     }
//     // reset
//     command(auth, url, apexis[0].base, apexis[0].args[0], logging);
//   } else {
//     command(auth, url, base, arg, logging);
//   }
//   return 0;
// }
