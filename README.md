# SmartTraffic - Arduino

Firmware para controle do semáforo inteligente.

---

## Estrutura

```
arduino/
├── arduino_code.ino # Código principal
├── hardware.png # Esquema de montagem (imagem)
├── LICENSE
└── README.md
```

---

## Como rodar

1. Verifique o esquema do arduino em `hardware.png`
2. Abra a **Arduino IDE**
3. Arquivo → Abrir → `smarttraffic_semaphore.ino`
4. **Tools → Board → Arduino Uno**
5. **Tools → Port** → selecione a porta USB
6. Clique em **Verify** (↓) e depois em **Upload** (→)

---

### Teste rápido

Abra o **Serial Monitor** e digite:

- `A` → Fecha o semáforo B, e abre o semáforo A 
- `B` → Fecha o semáforo A, e abre o semáforo B