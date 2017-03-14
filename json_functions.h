
/// Verifica se o campo json do token tok é igual a s
static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

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

void jstok_print(fstr base, jsmntok_t tok) {
  for (int i = 0; i < tok.end - tok.start; i++) {
    printf("%c", base[tok.start + i]);
  }
}