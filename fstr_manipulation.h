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
fstr fstr_replace(fstr base, const char *needle, const char *replacement) {
  char *match = strstr(base, needle);
  if (NULL == match) {
    return base;
  }
  int needle_len = strlen(needle);
  int replacement_len = strlen(replacement);
  fstr tmp;
  if (needle_len < replacement_len) {
    tmp = fat_new(str, fat_len(str, base) * 2);
    // tmp = calloc(1,sizeof(char)*strlen(base)*2);
    // se for usar sem a fstr
  } else {
    tmp = fat_new(str, fat_len(str, base));
    // tmp = fat_newfrom(str,base,fat_len(str,base));
  }
  int b_i = 0, t_i = 0;
  while (match != NULL) {
    memcpy(tmp + t_i, base + b_i, match - base - b_i);
    t_i += match - base - b_i;
    b_i += match - base - b_i;
    memcpy(tmp + t_i, replacement, replacement_len);
    t_i += replacement_len;
    b_i += needle_len;
    match = strstr(match + 1, needle);
  }
  memcpy(tmp + t_i, base + b_i, strlen(base + b_i));
  fat_setlen(str, tmp, b_i + strlen(base + b_i));
  fat_free(str, base);
  return tmp;
}

/// char**, com um pouco de açucar (vide fat_array.h), ou vector<string> com
/// macros
MAKEFAT(fstr);
FCMP(fstr, a, b) { return strcmp(a, b); }

// input must be null terminated, otherwise undefined behaviour shall occur,
// it's also destructive to the input string's dividers

/// Divide a fstr input todas as ocorrencias dos chars divider e retorna um
/// vetor de strings, ou ffstr.

// TODO: Arrumar os atributos de saida das fstr
ffstr fstr_explode(char input[], char divider[]) {
  // char *input = &input[0];
  char *divtmp = strstr(input, divider);
  size_t lendiv = strlen(divider);
  // size_t cont = 1;
  ffstr out = fat_new(fstr, 8);
  fat_push(fstr, out, input);
  while (divtmp != NULL) {
    // divtmp[0] = '\0';
    memset(divtmp,'\0',lendiv);
    out = fat_push(fstr, out, divtmp + lendiv);
    divtmp = strstr(divtmp + lendiv, divider);
  }
  return out;
}

///Recebe um vetor de strings normais, seu tamanho, um separador, e retorna uma fat string
fstr ffstr_join(char **input, int len, char sep) {
  int total_size = 0;
  for (int i = 0; i < len; i++) {
    total_size += strlen(input[i]);
  }
  fstr out =
      fat_new(str, total_size + len + 1); // separator is not put on the end
  for (int i = 0; i < len; i++) {
    for (int j = 0; input[i][j] != 0; j++) {
      out = fat_push(str, out, input[i][j]);
    }
    out = fat_push(str, out, sep);
  }
  out[fat_len(str, out) - 1] = '\0';
  // out = fat_push(str,out,'\0');
  return out;
}

/// lê o arquivo filename em uma fstr, caso stg seja NULL, retornará uma nova
/// fstr (que deve
/// ser liberada)
fstr fstr_from_file(fstr stg, const char *filename) {
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
