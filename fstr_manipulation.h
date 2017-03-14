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