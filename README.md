# XorOS

Минимальная bare-metal операционная система, написанная с нуля поверх самописного симулятора RISC-V.

---

## О проекте

Проект объединяет три репозитория через git subtree с общей целью — запустить ОС на самописном процессоре:

- [os/](os/) — bare-metal ядро XorOS (этот репозиторий)
- [rv32i/](rv32i/) — симулятор RISC-V RV32I ([Jigomas/rv32i](https://github.com/Jigomas/rv32i))
- [rv32i/cache_src/](rv32i/cache_src/) — реализация кэша ([Jigomas/LFU_cache](https://github.com/Jigomas/LFU_cache))

Симулятор и ядро связаны через бинарный интерфейс.

**Симулятор** (`rv32i/`) — программная модель процессора RISC-V, написанная на C++17.
Реализует базовую архитектуру RV32I с расширениями целочисленного умножения (M) и атомарных
операций (A). Работает в режиме single-cycle: один вызов `step()` — один цикл fetch → decode → execute.
Архитектура Von Neumann — единое адресное пространство для кода и данных. Для взаимодействия
с хостом симулятор перехватывает инструкцию `ecall` через C++ callback (`setEcallHandler`).
Между процессором и памятью можно вставить `CacheModel` — LRU-кэш, который собирает
статистику попаданий и промахов в реальном времени.

**Ядро ОС** (`os/`) — bare-metal программа на C11 и ассемблере, компилируется без libc.
Загрузчик (`boot.S`) инициализирует стек (`SP = 0x2000`) и вызывает `kernel_main`.
Симулятор загружает скомпилированный бинарник по адресу `0x0` и начинает исполнение с PC=0.
Вывод реализован через минимальный UART-драйвер (`uart.c`): запись по MMIO-адресу `0xF000`
перехватывается симулятором и выводится в stdout. Межпроцессное взаимодействие реализовано
через однонаправленный кольцевой буфер — pipe (`pipe.c`). Покрыта тестами.

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

**ECALL + MMIO UART.** Системные вызовы (`sys_putchar`, `sys_exit`) реализованы через `ecall`:
в симуляторе зарегистрирована syscall table — массив функций с индексом по `a7`. Параллельно
работает MMIO UART: `uart.c` пишет байт по адресу `0xF000`, симулятор перехватывает запись
и выводит символ в stdout. Ядро использует UART-драйвер напрямую, `sys_putchar` — в тестах.

**CacheModel как сменный слой памяти.** `RVModel` принимает тип памяти как шаблонный параметр
(`RVModel<XLEN, MemT>`), что позволяет подставить `CacheModel<32>` вместо `MemoryModel<32>`
без изменения ядра симулятора. `CacheModel` реализует LRU-кэш с политикой write-through
и read-allocate: промах при чтении загружает слово в кэш, запись всегда проходит насквозь
в `MemoryModel`. Размер кэша — 64 слова (256 байт). После исполнения симулятор печатает
статистику: количество попаданий, промахов и hit rate. При прогоне ядра XorOS получается
около 96% попаданий.

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

- `build/rv32i/rv32i_cpu`    — симулятор
- `os/build/xoros.bin`       — ядро OS в виде flat binary
- `os/build/xoros_tests.bin` — тесты OS

### Запуск ядра

```bash
./build/rv32i/rv32i_cpu os/build/xoros.bin
```

```plaintext
Hello from XorOS!
hi
kernel: all done

cache: 8227 hits / 1929 misses | 81.0% hit rate
```

### Запуск тестов OS

```bash
cmake --build os/build --target run_tests
```

```plaintext
=== XorOS ecall tests ===
[PASS] sys_putchar does not crash
[PASS] arithmetic: 1+1==2
[PASS] arithmetic: 10-3==7
[PASS] arithmetic: 10/3==3
[PASS] arithmetic: 10%3==1

--- vmem ---
[PASS] code runs after vmem_enable
[PASS] stack read after vmem_enable
[PASS] write+read via virtual addr
[PASS] arithmetic after vmem_enable: 6*7=42

--- canary ---
[PASS] STACK_CANARY == 0xDEADBEEF
[PASS] canary set on spawn
[PASS] canary intact after normal exit

--- pipe ---
[PASS] pipe_init: count == 0
[PASS] pipe_write returns 0
[PASS] pipe_read returns 0
[PASS] pipe_read correct byte
[PASS] pipe_read empty returns -1
[PASS] pipe_write full returns -1
[PASS] pipe wrap-around write ok
[PASS] pipe wrap-around read ok

passed: 20
failed: 0
```

### Запуск тестов симулятора

```bash
./build/rv32i/rv32i_tests
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
| ECALL             | Syscall table (`setEcallHandler`); a7=1 putchar, a7=10 halt   |
| Контекст          | `Context` (callee-saved) и `FullContext` (все 32 рег + PC)    |
| Память            | Плоская, little-endian; LR/SC reservation; MMIO regions       |
| CacheModel        | LRU-кэш (64 слова) поверх MemoryModel; write-through; hit/miss|
| Дизассемблер      | `Disasm::disassemble()` — DecodedInstr в строку мнемоники     |
| Отладка           | Трассировка инструкций (`setDebug`), hex-дамп памяти          |

### ОС (`os/`)

| Компонент        | Описание                                                                   |
|------------------|----------------------------------------------------------------------------|
| `boot.S`         | Инициализация SP, GP, mtvec, mret (M->S mode), вызов `kernel_main`         |
| `ecall.h`        | `sys_putchar` / `sys_exit` через inline asm                                |
| `csr.h`          | Адреса Machine-level CSR, коды mcause                                      |
| `trap.S`         | Точка входа trap-хендлера: сохранение caller-saved, `mret`                 |
| `trap.c`         | Диспетчеризация исключений по mcause, вывод причины, паника                |
| `kalloc.h/c`     | Bump allocator: статический heap[4×4KiB], PAGE_SIZE=4096                   |
| `spinlock.h`     | Header-only spinlock через `amoswap.w.aq` / `amoswap.w.rl` (A-ext)         |
| `process.h`      | PCB: `proc_state_t`, `context_t` (callee-saved), `proc_t`                  |
| `scheduler.h/c`  | Планировщик round-robin: `sched_init/spawn/yield/exit`                     |
| `sched_switch.S` | `context_switch` — сохранение/восстановление ra/sp/s0-s11                  |
| `vmem.h/c`       | Sv32 виртуальная память: identity map, satp, sfence.vma (NOP)              |
| `pipe.h/c`       | Кольцевой буфер (16 байт); non-blocking pipe_write/read                    |
| `uart.h/c`       | MMIO UART: uart_putchar/puts → запись в 0xF000                             |
| `kernel_main`    | Демо pipe + UART: task_a пишет в pipe, task_b читает через UART            |
| Тесты            | 27 тестов: putchar, арифметика, Sv32 vmem, canary, pipe, kalloc, spinlock  |

> Список процессов в планировщике планируется расширить до динамического array-of-slots без malloc на основе [Jigomas/List](https://github.com/Jigomas/List).

---

## Дорожная карта

### Симулятор

- [ ] CLINT — mtime/mtimecmp по MMIO; нужен для таймерного прерывания
- [ ] уровни привилегий в CPU — текущий режим M/S/U; Illegal Instruction при CSR-доступе из U-режима
- [ ] ecall через fireTrap — убрать C++ колбек, пустить ecall → fireTrap → mtvec → trap_handler
- [ ] ELF-загрузчик

### ОС

- [ ] U-mode + CAUSE_ECALL_U handler — системные вызовы без паники
- [ ] полная 32-регистровая рамка в trap.S — нужна для preemption
- [ ] per-process page tables — изоляция процессов; требует поля satp в process.h
- [ ] kfree() + свободный список страниц
- [ ] таймерное прерывание → вытесняющий планировщик (требует CLINT)
- [ ] fork + wait
- [ ] COW-форк — ref-count в kalloc, sc.w для fault handler
- [ ] Динамический список процессов — array-of-slots без malloc (на основе [Jigomas/List](https://github.com/Jigomas/List))
- [ ] CoreMark — bare-metal бенчмарк производительности

## Литература, без которой реализация проекта была бы невозможной

- Harris & Harris — *Digital Design and Computer Architecture: RISC-V Edition*
- *The RISC-V Instruction Set Manual, Volume I: Unprivileged Architecture*
- *The RISC-V Instruction Set Manual, Volume II: Privileged Architecture*
- Cox, Kaashoek, Morris — *xv6: a simple, Unix-like teaching operating system*
- Arpaci-Dusseau — *Operating Systems: Three Easy Pieces*
- Corbet, Rubini, Kroah-Hartman — *Linux Device Drivers, 3rd Edition*
