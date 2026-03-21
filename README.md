# XorOS

Минимальная bare-metal операционная система, написанная с нуля поверх самописного симулятора RISC-V.

---

## О проекте

Проект состоит из двух независимых частей, связанных через бинарный интерфейс.

**Симулятор** (`rv32i/`) — программная модель процессора RISC-V, написанная на C++17.
Реализует базовую архитектуру RV32I с расширениями целочисленного умножения (M) и атомарных
операций (A). Работает в режиме single-cycle: один вызов `step()` — один цикл fetch → decode → execute.
Архитектура Von Neumann — единое адресное пространство для кода и данных. Для взаимодействия
с хостом симулятор перехватывает инструкцию `ecall` через C++ callback (`setEcallHandler`).

**Ядро ОС** (`os/`) — bare-metal программа на C11 и ассемблере, компилируется без libc.
Загрузчик (`boot.S`) инициализирует стек (`SP = 0x2000`) и вызывает `kernel_main`.
Симулятор загружает скомпилированный бинарник по адресу `0x0` и начинает исполнение с PC=0.
Единственный интерфейс с внешним миром — два системных вызова через `ecall`:
`sys_putchar` (вывод символа) и `sys_exit` (завершение). Покрыта тестами.

```plaintext
┌─────────────────────────────────────────┐
│               XorOS (os/)               │
│  boot.S → kernel_main.c  │  тесты       │
│  ecall.h (sys_putchar / sys_exit)       │
└─────────────────────────────────────────┘
               │
               │  ecall  /  flat binary (0x0)
               │
┌─────────────────────────────────────────┐
│         rv32i симулятор (rv32i/)        │
│  RV32I + M-ext + A-ext                  │
│  single-cycle · Von Neumann · 64 KiB    │
└─────────────────────────────────────────┘
```

### Архитектурные решения

**Single-cycle, Von Neumann.** Один `step()` — один fetch/decode/execute без кэша и конвейера.
Единая память для кода и данных. Состояние процессора полностью определяется значением PC
и регистрового файла — это упрощает отладку ОС по сравнению с реальным железом.

**Flat binary вместо ELF.** ОС компонуется с флагами `-Ttext=0x0 -e _start` в сырой
бинарник без заголовков. Симулятор копирует его в начало виртуальной памяти напрямую —
никакого ELF-загрузчика. Стек располагается по адресу `0x2000` и растёт вниз в пределах 64 KiB.

**ECALL вместо MMIO.** Вместо эмуляции UART по фиксированному адресу симулятор
перехватывает `ecall` через C++ callback. ОС не привязана к адресу конкретного устройства
и легко переносима между конфигурациями симулятора.

---

## Состав репозитория

```plaintext
XorOS/
├── rv32i/                  — симулятор RISC-V (C++17): RV32I + M + A расширения
├── os/                     — bare-metal ядро для симулятора (C11 + asm, riscv64-unknown-elf-gcc)
├── CMakeLists.txt          — корневой CMake: сборка rv32i + os как подпроекты
├── compile_commands.json   — слитый compile_commands (rv32i + os) для clangd
└── TODO.txt                — список задач
```

Подробнее:

- [rv32i/README.md](rv32i/README.md) — симулятор: инструкции, API, примеры
- [os/README.md](os/README.md)       — ядро: загрузка, системные вызовы, сборка

---

## Быстрый старт

### Зависимости

```bash
# C++17 компилятор (GCC ≥ 9 или Clang ≥ 10)
# CMake ≥ 3.16
# Кросс-компилятор для OS:
sudo apt-get install gcc-riscv64-unknown-elf
```

### Сборка

```bash
git clone <repo>
cd XorOS
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

Собирается:

- `rv32i/build/rv32i_cpu`    — симулятор
- `os/build/xoros.bin`       — ядро OS в виде flat binary
- `os/build/xoros_tests.bin` — тесты OS

### Запуск ядра

```bash
./rv32i/build/rv32i_cpu os/build/xoros.bin
```

```plaintext
Hello from XorOS!
```

### Запуск тестов OS

```bash
cmake --build build --target run_tests
```

```plaintext
=== XorOS ecall tests ===
[PASS] sys_putchar does not crash
[PASS] arithmetic: 1+1==2
[PASS] arithmetic: 10-3==7
[PASS] arithmetic: 10/3==3
[PASS] arithmetic: 10%3==1

passed: 5
failed: 0
```

### Запуск тестов симулятора

```bash
./rv32i/build/rv32i_tests
```

---

## Что реализовано

### Симулятор (`rv32i/`)

| Компонент         | Описание                                                      |
|-------------------|---------------------------------------------------------------|
| RV32I             | Полный набор инструкций: ALU, BRANCH, LOAD/STORE, JAL/JALR    |
| M-extension       | MUL / MULH / MULHSU / MULHU / DIV / DIVU / REM / REMU         |
| A-extension       | LR.W / SC.W + AMO(SWAP/ADD/XOR/AND/OR/MIN/MAX/MINU/MAXU)      |
| CSR               | CSRRW / CSRRS / CSRRC / CSRRWI / CSRRSI / CSRRCI              |
| ECALL             | Callback-хендлер (`setEcallHandler`); a7=1 putchar, a7=10 halt|
| Контекст          | `Context` (callee-saved) и `FullContext` (все 32 рег + PC)    |
| Память            | Плоская, little-endian; LR/SC reservation                     |
| Отладка           | Трассировка инструкций (`setDebug`), hex-дамп памяти          |

### ОС (`os/`)

| Компонент     | Описание                                              |
|---------------|-------------------------------------------------------|
| `boot.S`      | Инициализация SP=0x2000, вызов `kernel_main`          |
| `ecall.h`     | `sys_putchar` / `sys_exit` через inline asm           |
| `csr.h`       | Адреса Machine-level CSR, коды mcause (заготовка)     |
| `kernel_main` | Вывод "Hello from XorOS!" + sys_exit                  |
| Тесты         | 5 тестов: putchar, арифметика (+, −, /, %)            |

---

## Дорожная карта

### Симулятор

- [ ] ELF-загрузчик
- [ ] Дизассемблер (`DecodedInstr` → строка)
- [ ] MMIO через callback-map в `MemoryModel`

### ОС

- [ ] `trap.h` / `trap.c` — обработка исключений и прерываний
- [ ] `process.h` — PCB (Process Control Block)
- [ ] `scheduler.h` / `scheduler.c` — планировщик round-robin
- [ ] `vmem.h` / `vmem.c` — виртуальная память Sv32

## Литература, без которой реализация проекта была бы невозможной

- Harris & Harris — *Digital Design and Computer Architecture: RISC-V Edition*
- *The RISC-V Instruction Set Manual, Volume I: Unprivileged Architecture*
- *The RISC-V Instruction Set Manual, Volume II: Privileged Architecture*
- Cox, Kaashoek, Morris — *xv6: a simple, Unix-like teaching operating system*
- Arpaci-Dusseau — *Operating Systems: Three Easy Pieces*
- Corbet, Rubini, Kroah-Hartman — *Linux Device Drivers, 3rd Edition*
