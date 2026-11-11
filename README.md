# SmartTraffic — firmware ST1 da maquete

Máquina de estados sem `delay()`, com verde mínimo/máximo, amarelo contínuo, intervalo com ambos vermelhos e ciclo fixo na perda de comunicação. Mantém a pinagem existente:

| Via | Verde | Amarelo | Vermelho |
|---|---|---|---|
| A | 9 | 10 | 11 |
| B | 3 | 5 | 6 |

## Carregar

1. Encerre o Python para liberar a serial.
2. Abra `arduino_code.ino` na Arduino IDE. Se a IDE pedir, coloque o arquivo em uma pasta chamada `arduino_code` (mesmo nome do sketch).
3. Selecione a placa e a porta da montagem. A compilação desta entrega foi verificada para Arduino Uno (`arduino:avr:uno`).
4. Verifique e carregue o sketch. Feche o Monitor Serial antes de executar o Python.
5. Execute `python Teste_camera.py` em `computer-vision`. Porta padrão COM5; ajuste `SMARTTRAFFIC_ARDUINO_PORT` quando necessário.

O firmware inicia com ambos vermelhos por 1 segundo e passa ao ciclo fixo, começando por A. A inicialização não depende do Python. O firmware anterior, que recebia letras A/B isoladas, não é compatível com o novo controlador.

## Regras

- Verdes de 8 a 25 segundos; amarelo de 3 segundos; ambos vermelhos por 1 segundo entre vias.
- Automático: os tempos enviados pelo Python atualizam o plano sem reiniciar a fase atual. Ao vencer o tempo, alterna para a outra via.
- Manual: uma solicitação de troca aguarda o mínimo de 8 segundos; mesmo sem nova solicitação alterna ao atingir 25 segundos. Repetir um pedido não prolonga a fase.
- Transições aceitas são concluídas antes de outra troca. Nenhum comando cancela amarelo ou intervalo vermelho.
- Sem comando válido por 4 segundos, assume FALLBACK com 12 segundos por via. Não reinicia a fase nem interrompe uma transição já aceita.
- Se o Python parar, a placa energizada continua operando. Desconectar uma placa alimentada somente por USB também corta sua energia; isso não testa o watchdog.

## Protocolo serial ST1

9600 baud, ASCII, uma linha por comando terminada por `\n`. IDs são inteiros positivos. O Python envia um comando por vez e aguarda confirmação antes do próximo.

| Comando | Efeito |
|---|---|
| `AUTO 1 14000 12000` | ID 1; modo automático; verdes A=14 s e B=12 s |
| `MANUAL 2 B` | ID 2; modo manual; solicita B respeitando a fase atual |
| `KEEP 3` | ID 3; mantém comunicação sem alterar plano, modo ou relógio |

Tempos AUTO fora de 8000–25000 ms são rejeitados. MANUAL durante amarelo/vermelho ou troca pendente é rejeitado. Mensagens legadas, desconhecidas ou maiores que o buffer não acionam sinais.

Estado emitido a cada 250 ms, nas transições e após comandos:

```text
STATE ST1 2 A_YELLOW 500 3000 MANUAL B
```

Campos: versão, último ID aceito, fase (`A_GREEN`, `A_YELLOW`, `B_GREEN`, `B_YELLOW`, `ALL_RED`), milissegundos desde o começo da fase, duração atual planejada, modo (`AUTO`, `MANUAL`, `FALLBACK`), via de destino. O mesmo ID no estado confirma a aceitação do comando; somente a fase informa que a transição terminou. Não comprova o estado elétrico dos LEDs.

Rejeição: `ERROR ST1 <id> comando_invalido_ou_transicao`. Um comando rejeitado não renova o watchdog. O Python interrompe comandos quando o estado ou a confirmação expira; a placa assume FALLBACK.

## Ensaios físicos pendentes

Verifique início com ambos vermelhos; A→amarelo→ambos vermelhos→B e sequência inversa; ausência de verdes simultâneos; limites mínimo/máximo; pedidos repetidos; modo manual durante transições; parada do Python; reconexão/reinício da placa e do Python. Registre configuração, tempos observados e resultado. Execute também o checklist de câmera e controle em `../computer-vision/README.md`.

Firmware exclusivo da maquete acadêmica. Não possui realimentação elétrica dos LEDs.
