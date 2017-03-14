#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

// #include <dirent.h>                  // diretórios

// #include "./deps/b64/b64.h"             // fazer encode do base64
#include "./deps/fat_array/fat_array.h" // array dinâmica
#include "./deps/flag/flag.h"           // lidar com argv e argc
#include "./deps/jsmn/jsmn.h"           // json parser lib

#include <curl/curl.h> // para comunicação web no geral

#define VERSION "0.0.2" // pré-git

#include "external.h"
#include "fstr_manipulation.h"
#include "json_functions.h"
#include "base_types.h"
#include "base_functions.h"


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
