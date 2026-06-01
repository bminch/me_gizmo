#include "me_gizmo.h"
#include "parser.h"

void setup() {
  init_me_gizmo();
  init_parser();
}

void loop() {
  parser_state();
}
