#include <stdio.h>
#include <string.h>
#include "./deps/fat_array/fat_array.h"

typedef char str;

MAKEFAT(str);

// size_t revstrstr(const char *haystack, const char *needle,
//                  const size_t haystacklen) {
//   static const char *current;
//   static size_t left;
//   if (haystack != NULL) {
//     current = haystack;
//     left = haystacklen - 1;
//   }
//   size_t needle_len = strlen(needle);
//   for (int j; left >= needle_len; left--) {
//     // printf("%c == %c\n",current[left],needle[needle_len-1]);
//     for (j = 0;
//          current[left - j] == needle[needle_len - 1 - j] && j < needle_len;
//          j++) {
//       // printf("j:%d\n",j);
//     }
//     if (j == needle_len) {
//       left--;
//       return left + 1;
//     }
//   }
//   return -1; // will explode if tried to assign on
// }

// fstr fstr_replace(fstr base, const char *needle, const char *substitute) {
//   int cont_needle = 0;
//   for (char *tmp = strstr(base, needle);
//        tmp != NULL; /// counts the number of occurences of needle
//        tmp = strstr(tmp + 1, needle)) {
//     // printf("%s\n",tmp);fflush(stdout);
//     cont_needle += 1;
//   }
//   if (cont_needle == 0) {
//     return base;
//   }
//   int needle_len = strlen(needle);
//   int substitute_len = strlen(substitute);
//   // ensure we have space
//   base = fat_growzero(str, base, (substitute_len - needle_len) * cont_needle);
//   // copy things
//   int k = (substitute_len - needle_len) * cont_needle;
//   int next = revstrstr(base, needle, fat_len(str, base));
//    printf("needle: %i,subst: %i\n",needle_len,substitute_len);
//    printf("k:%d, next: %d\n",k,next);
//   for (int i = fat_len(str, base) + k; i >= 0; i--) {
//      printf("%d; base[i] = %c, base[i-k] =%c\n",i,base[i],base[i-k]);fflush(stdout);
//     base[i] = base[i - k];
//     if (i - k == next) {
//          printf("WOO!");fflush(stdout);
//       i -= substitute_len - 1;
//       k -= (substitute_len - needle_len);
//       if (i < 0) {
//         i = 0;
//       }
//       memcpy(&base[i], substitute, substitute_len);
//       next = revstrstr(NULL, needle, 0);
//        printf("k:%d, next: %d, i%d\n",k,next,i);fflush(stdout);
//     }
//   }
//   return base;
// }

fstr replace(fstr base, const char* needle, const char* replacement){
    char* match = strstr(base,needle);
    if(NULL == match){
        return base;
    }
    int needle_len = strlen(needle);
    int replacement_len = strlen(replacement);
    fstr tmp;
    if(needle_len < replacement_len){
        tmp = fat_new(str,fat_len(str,base)*2);
        // tmp = fat_newfrom(str,base,fat_len(str,base)*2);
    }else{
        tmp = fat_new(str,fat_len(str,base));
        // tmp = fat_newfrom(str,base,fat_len(str,base));
    }
    int b_i = 0, t_i = 0;
    while(match != NULL){
        memcpy(tmp+t_i,base+b_i,match-base-b_i);
        t_i+=match-base-b_i;
        b_i+=match-base-b_i;
        memcpy(tmp+t_i,replacement,replacement_len);
        t_i+=replacement_len;
        b_i+=needle_len;
        match = strstr(match+1,needle);
    }
    memcpy(tmp+t_i,base+b_i,strlen(base+b_i));
    fat_free(str,base);
    return tmp;
}

int main(){
  const char* test_base = "inicio 12345 1and1 12345 2and2 12345 3and3 12345 resto resto";
  fstr test = fstr_newfrom(test_base,strlen(test_base));
  printf("%s:%s",test,replace(test,"12345","543")); //leak!
  return 0;;
}