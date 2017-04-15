// include in fat_array.h? /*

// int fat_savehdr(FILE* restrict f,void* fat_array){
//   if(!fat_array){
//     fat_array = &(fat_hdr){.len=0,.alloc=0,.flags=0}; // this is fine!?
//   }else{
//     fat_array = fat_toStruct(voidp__,fat_array);
//   }
//   printf("alloc: %zu, len: %zu, flags: %x\n",((struct fstr_struct*)fat_array)->alloc,((struct fstr_struct*)fat_array)->len,((struct fstr_struct*)fat_array)->flags);
//   return fwrite(fat_array,sizeof(fat_hdr),1,f);
// }

// fat_hdr fat_loadhdr(FILE* restrict f){
//   fat_hdr out;
//   fread(&out,sizeof(fat_hdr),1,f);
//   return out;
// }


// */

static inline int fstr_save(FILE* restrict f,fstr stg){
  int writen = 0;
  // writen += fat_savehdr(f,stg);
  if(stg){
    writen += fat_save(str,f,stg);
    writen += fwrite(stg,sizeof(str),fat_len(str,stg),f);
  }else{
    fat_struct(fstr) tmp = {.len = 0, .alloc = 0, .flags = 0};
    writen += fat_save(str,f,&tmp.vec);
  }
  return writen;
}

static inline fstr fstr_load(FILE* restrict f){
  // fat_hdr tmp = fat_loadhdr(f);
  fat_struct(str) tmp;
  fat_load(str,f,tmp);
  fstr out = fat_new(str,tmp.alloc);
  memcpy(fat_toStruct(str,out),&tmp,sizeof(fat_struct(str)));
  // *(fat_hdr*)fat_toStruct(str,out) = tmp;
  // fat_setlen(str,out,tmp.len);
  // fat_setflags(str,out,tmp.flags);
  fread(out,sizeof(str),tmp.len,f);
  return out;
}

static int ffstr_save(FILE* restrict f,ffstr stgvec){
  int writen = 0;
  // writen += fat_savehdr(f,stgvec);
  if(stgvec){
    writen += fat_save(fstr,f,stgvec);
    for(size_t i = 0; i < fat_len(fstr,stgvec); i++){
      writen += fstr_save(f,stgvec[i]);
    }
  }else{ //vetor vazio, escrevemos um header vazio
    fat_struct(fstr) tmp ={.len=0,.alloc=0,.flags=0};
    writen += fat_save(fstr,f,&tmp.vec);
  }
  return writen;
}

static ffstr ffstr_load(FILE* restrict f){
  fat_struct(fstr) tmp;
  fat_load(fstr,f,tmp);
  ffstr out = fat_new(fstr,tmp.alloc);
  // *((fat_hdr*)fat_toStruct(fstr,out)) = tmp;
  memcpy(fat_toStruct(fstr,out),&tmp,sizeof(fat_struct(fstr)));
  for(size_t i = 0; i < tmp.len; i++){
    out[i] = fstr_load(f);
  }
  return out;
}

static int request_save(FILE* restrict f,request rqst){
  int writen = 0;
  writen += fwrite(&rqst.fact_to_next,sizeof(double),1,f);
  writen += fstr_save(f,rqst.name);
  writen += fstr_save(f,rqst.next_cmd);
  writen += fstr_save(f,rqst.prev_cmd);
  writen += fstr_save(f,rqst.base);
  writen += ffstr_save(f,rqst.headers);
  writen += ffstr_save(f,rqst.args);
  return writen;
}

static request request_load(FILE* restrict f){
  request out;
  fread(&(out.fact_to_next),sizeof(double),1,f);
  out.name = fstr_load(f);
  out.next_cmd = fstr_load(f);
  out.prev_cmd = fstr_load(f);
  out.base = fstr_load(f);
  out.headers = ffstr_load(f);
  out.args = ffstr_load(f);
  return out;
}

static int frequest_save(FILE* restrict f, frequest frqst){
  int writen = 0;
  if(frqst){
    writen += fat_save(request,f,frqst);
    for(size_t i = 0; i < fat_len(request,frqst); i++){
      writen += request_save(f,frqst[i]);
    }
  }else{
    fat_struct(request) tmp = {.len = 0, .alloc = 0, .flags = 0};
    fat_save(request,f,&tmp.vec);
  }
  return writen;
}

static frequest frequest_load(FILE* restrict f){
  // fat_hdr tmp = fat_loadhdr(f);
  fat_struct(request) tmp;
  fat_load(request,f,tmp);
  frequest out = fat_new(request,tmp.alloc);
  // *fat_toStruct(request,out) = tmp;
  memcpy(fat_toStruct(request,out),&tmp,sizeof(fat_struct(request)));
  for(size_t i = 0; i < tmp.len; i++){
    out[i] = request_load(f);
  }
  return out;
}

static int cam_cfg_save(FILE* restrict f,cam_cfg* cfg){
  int writen = fstr_save(f,cfg->name);
  writen += ffstr_save(f,cfg->headers);
  writen += frequest_save(f,cfg->requests);
  return writen;
}

static cam_cfg* cam_cfg_load(FILE* restrict f){
  cam_cfg* out = malloc(sizeof(cam_cfg));
  // TODO: check for out of memory
  if(out){
    out->name = fstr_load(f);
    out->headers = ffstr_load(f);
    out->requests = frequest_load(f);
  }
  return out;
}

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

/// A grande função que lê a configuração da câmera de nome config,
fkeyval read_camera_args(const char *config) {
  char cam_name[1 << 7];
  strcpy(&cam_name[0], "./cameras/");
  strcat(&cam_name[0], config);
  strcat(&cam_name[0], ".json");
  fstr cam_cfg_txt = fstr_from_file(NULL, cam_name);

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

// void print20(char* stg){
//   for(int i = 0; i < 20; i++,stg++){
//     putchar(stg[0]);
//   }
//   return;
// }

/// lê o tipo da câmera da pasta /config/cam_type.json
cam_cfg *read_cam_config(const char *cam_type) {
  char tmp[1 << 7];
  strcpy(tmp, "./config/");
  strcat(tmp, cam_type);
  strcat(tmp, ".json");

  fstr cam_cfg_txt = fstr_from_file(NULL, tmp);
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
      i++;
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
        if (jstokens[i].type != JSMN_OBJECT) {
          fprintf(stderr,
                  "commands should have an array of objects (requests), it "
                  "does not, currently, it has type %d; file %s at line %i\n",
                  jstokens[i].type, cam_type, __LINE__);
          // print20(&cam_fg_txt[jstokens[i].start]);
          goto cleanup_error2;
        }
        int rqst_size = jstokens[i].size;
        // printf("js_cmd: ");
        // jstok_print(cam_cfg_txt, jstokens[i]);
        // puts("");
        // printf("rqst_size = %d\n",rqst_size);
        i++;
        for (int k = 0; k < rqst_size; k++) { // parsing each request
          // printf("rqst:");
          // print20(&cam_cfg_txt[jstokens[i].start]);
          // puts("");
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
            i++;
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
            i++;
            // printf("cmd_base = %s\n",rqst.base);
          } else if (0 == jsoneq(cam_cfg_txt, &jstokens[i], "factor_to_next")) {
            i++;
            // printf("cmd_fact_to_next: type: %i, start: %i, end: %i, size:\
            // %i\n",
            //        jstokens[i].type, jstokens[i].start, jstokens[i].end,
            //        jstokens[i].size);
            // test if null
            if (jstokens[i].type == JSMN_PRIMITIVE) {
              if (cam_cfg_txt[jstokens[i].start] == 'n') {
                rqst.fact_to_next = 0.0;
              } else { // should also test if it's not a number
                tmpstr = fstr_from_jstok(cam_cfg_txt, jstokens[i]);
                rqst.fact_to_next = strtod(tmpstr, NULL);
                // printf("TO_NEXT,%lf\n",rqst.fact_to_next);
                fat_free(str, tmpstr);
                i++;
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
            i++;
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
              i++;
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
                i++;
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
              // printf("arg_after:");
              // print20(&cam_cfg_txt[jstokens[i].start]);
              // puts("");            
          } else {
              fprintf(stderr,
                      "args should be either null, or an array containing the "
                      "args as strings, in file %s, at line %i\n",
                      cam_type, __LINE__);
            }
          } // new commands could go here, prehaps another field, maybe
            // adressable as a "macro"
          else{
            printf("\nwat\n");
            jstok_print(cam_cfg_txt,jstokens[i]);
          }
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

// checksum simples
uint16_t checksum(const char* file){
	char readtmp[1024];
	uint16_t cksum = 0;
	int read;
	FILE *f = fopen(file,"rb");
	while(!feof(f)){
		read = fread(readtmp,sizeof(char),1024,f);
		for(int i = 0; i < read; i++){
			cksum += (uint8_t) readtmp[i];
		}
		cksum &= 0xFFFFFFFF; //fast modulo
		//cksum %= 0xFFFF
	}
	return cksum;
}

uint16_t cksum_file(const char* file){
  char tmp[1<<7];
  strcpy(tmp,"./cameras/");
  strcat(tmp,file);
  strcat(tmp,".json");
  uint16_t out = checksum(tmp);
  return out;
}

int valid_cache(uint16_t cksum, const char* file){
  char tmp[1<<7];
  strcpy(tmp,"./cameras/");
  strcat(tmp,file);
  strcat(tmp,".cache");
  FILE *f = fopen(tmp,"rb");
  if(f){
    uint16_t old_cksum;
    fread(&old_cksum,sizeof(uint16_t),1,f);
    if(old_cksum == cksum){
      fclose(f);
      return 1;
    }else{
      fclose(f);
      return 0;
    }
  }else{//no such file
    return 0;
  }
}

cam_cfg* load_cam_cache(const char* file){
  char tmp[1<<7];
  strcpy(tmp,"./cameras/");
  strcat(tmp,file);
  strcat(tmp,".cache");
  FILE *f = fopen(tmp,"rb");
  fread(tmp,sizeof(uint16_t),1,f); //ignore checksum
  cam_cfg *out = cam_cfg_load(f);
  fclose(f);
  return out;
}

void save_cam_cache(uint16_t cksum, const char* file,cam_cfg* cfg){
  char tmp[1<<7];
  strcpy(tmp,"./cameras/");
  strcat(tmp,file);
  strcat(tmp,".cache");
  FILE *f = fopen(tmp,"wb");
  if(!f){
    fprintf(stderr,"Error opening file %s for writing cache, on line %d",file,__LINE__);
    return;
  }else{
    fwrite(&cksum,sizeof(uint16_t),1,f);
    cam_cfg_save(f,cfg);
    fclose(f);
  }
  return;
}
