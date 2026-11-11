// SmartTraffic ST1: controlador local da maquete, sem delay().
// A: verde 9, amarelo 10, vermelho 11. B: verde 3, amarelo 5, vermelho 6.
#include <stdio.h>
#include <string.h>


const byte GREEN_A = 11, YELLOW_A = 10, RED_A = 9;
const byte GREEN_B = 6, YELLOW_B = 5, RED_B = 3;
const unsigned long MIN_GREEN = 8000, MAX_GREEN = 25000, FIXED_GREEN = 12000;
const unsigned long YELLOW_TIME = 3000, CLEARANCE = 1000, WATCHDOG = 4000;
enum Phase { A_GREEN, A_YELLOW, B_GREEN, B_YELLOW, ALL_RED };
enum Mode { AUTO, MANUAL, FALLBACK };
Phase phase = ALL_RED;
Mode mode = FALLBACK;
char target = 'A';
unsigned long phaseStarted = 0, lastCommand = 0, lastReport = 0;
unsigned long greenA = FIXED_GREEN, greenB = FIXED_GREEN, acceptedSeq = 0;
char input[64];
byte inputLength = 0;
bool overflow = false;

const char* phaseName() {
  switch (phase) {
    case A_GREEN: return "A_GREEN";
    case A_YELLOW: return "A_YELLOW";
    case B_GREEN: return "B_GREEN";
    case B_YELLOW: return "B_YELLOW";
    default: return "ALL_RED";
  }
}

void enterPhase(Phase next, unsigned long now) {
  // Retira primeiro todos os verdes/amarelos, antes de liberar qualquer via.
  digitalWrite(GREEN_A, LOW); digitalWrite(GREEN_B, LOW);
  digitalWrite(YELLOW_A, LOW); digitalWrite(YELLOW_B, LOW);
  digitalWrite(RED_A, HIGH); digitalWrite(RED_B, HIGH);
  phase = next;
  phaseStarted = now;
  if (next == A_GREEN || next == A_YELLOW) {
    digitalWrite(RED_A, LOW);
    digitalWrite(next == A_GREEN ? GREEN_A : YELLOW_A, HIGH);
  } else if (next == B_GREEN || next == B_YELLOW) {
    digitalWrite(RED_B, LOW);
    digitalWrite(next == B_GREEN ? GREEN_B : YELLOW_B, HIGH);
  }
}

unsigned long phaseDuration() {
  if (phase == ALL_RED) return CLEARANCE;
  if (phase == A_YELLOW || phase == B_YELLOW) return YELLOW_TIME;
  char road = phase == A_GREEN ? 'A' : 'B';
  if (target != road) return MIN_GREEN;
  if (mode == MANUAL) return MAX_GREEN;
  return road == 'A' ? greenA : greenB;
}

void report(unsigned long now) {
  Serial.print("STATE ST1 "); Serial.print(acceptedSeq); Serial.print(' ');
  Serial.print(phaseName()); Serial.print(' ');
  Serial.print(now - phaseStarted); Serial.print(' ');
  Serial.print(phaseDuration()); Serial.print(' ');
  Serial.print(mode == AUTO ? "AUTO" : mode == MANUAL ? "MANUAL" : "FALLBACK");
  Serial.print(' '); Serial.println(target);
  lastReport = now;
}

void command(char* line, unsigned long now) {
  char operation[12], road, extra;
  unsigned long seq, a, b;
  bool valid = false;
  if (sscanf(line, "%11s %lu", operation, &seq) != 2 || seq == 0) return;
  if (strcmp(operation, "AUTO") == 0 &&
      sscanf(line, "%11s %lu %lu %lu %c", operation, &seq, &a, &b, &extra) == 4 &&
      a >= MIN_GREEN && a <= MAX_GREEN && b >= MIN_GREEN && b <= MAX_GREEN) {
    // Uma troca manual já iniciada deve terminar antes de seguir o plano automático.
    greenA = a; greenB = b; mode = AUTO; valid = true;
  } else if (strcmp(operation, "MANUAL") == 0 &&
             sscanf(line, "%11s %lu %c %c", operation, &seq, &road, &extra) == 3 &&
             (road == 'A' || road == 'B') &&
             (phase == A_GREEN || phase == B_GREEN)) {
    // Não redireciona uma troca que já foi aceita.
    char current = phase == A_GREEN ? 'A' : 'B';
    if (target == current) { mode = MANUAL; target = road; valid = true; }
  } else if (strcmp(operation, "KEEP") == 0 &&
             sscanf(line, "%11s %lu %c", operation, &seq, &extra) == 2) {
    valid = true;
  }
  if (valid) { acceptedSeq = seq; lastCommand = now; }
  else {
    Serial.print("ERROR ST1 "); Serial.print(seq);
    Serial.println(" comando_invalido_ou_transicao");
  }
  report(now);
}

void setup() {
  pinMode(GREEN_A, OUTPUT); pinMode(YELLOW_A, OUTPUT); pinMode(RED_A, OUTPUT);
  pinMode(GREEN_B, OUTPUT); pinMode(YELLOW_B, OUTPUT); pinMode(RED_B, OUTPUT);
  enterPhase(ALL_RED, millis());
  Serial.begin(9600);
  report(millis());
}

void loop() {
  unsigned long now = millis();
  // Limita trabalho serial por iteração para não atrasar a máquina de estados.
  byte budget = 64;
  while (Serial.available() && budget--) {
    char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      if (!overflow) { input[inputLength] = '\0'; command(input, now); }
      inputLength = 0; overflow = false;
    } else if (inputLength < sizeof(input) - 1 && !overflow) {
      input[inputLength++] = c;
    } else { overflow = true; }
  }
  if (mode != FALLBACK && now - lastCommand >= WATCHDOG) {
    mode = FALLBACK; greenA = greenB = FIXED_GREEN;
    // Uma transição aceita é concluída; nenhum verde/amarelo é reiniciado.
    report(now);
  }
  if (now - phaseStarted >= phaseDuration()) {
    switch (phase) {
      case A_GREEN: target = 'B'; enterPhase(A_YELLOW, now); break;
      case B_GREEN: target = 'A'; enterPhase(B_YELLOW, now); break;
      case A_YELLOW:
      case B_YELLOW: enterPhase(ALL_RED, now); break;
      case ALL_RED: enterPhase(target == 'A' ? A_GREEN : B_GREEN, now); break;
    }
    report(now);
  } else if (now - lastReport >= 250) { report(now); }
}
