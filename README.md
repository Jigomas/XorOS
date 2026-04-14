# XorOS

![logo](logo.png)

Минимальная bare-metal операционная система, написанная с нуля поверх самописного симулятора RISC-V.

![C11](https://img.shields.io/badge/C-C11-blue)
![C++17](https://img.shields.io/badge/C%2B%2B-17-blue)
![RISC-V](https://img.shields.io/badge/arch-RISC--V%20RV32IA-green)
![CMake](https://img.shields.io/badge/build-CMake-informational)

---

## Содержание

- [О проекте](#о-проекте)
- [Состав репозитория](#состав-репозитория)
- [Быстрый старт](#быстрый-старт)
- [Что реализовано](#что-реализовано)
- [Дорожная карта](#дорожная-карта)
- [Литература](#литература-без-которой-реализация-проекта-была-бы-невозможной)

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
Архитектура Von Neumann — единое адресное пространство для кода и данных. Симулятор отслеживает
уровень привилегий (`PrivMode` M/S/U): `mret` восстанавливает его из `mstatus.MPP`, а `ecall`
кидает `fireTrap(EXC_ECALL_U/S/M)` — управление напрямую уходит на `mtvec` в ОС.
Между процессором и памятью вставлен `CacheModel` — LRU-кэш, который собирает
статистику попаданий и промахов в реальном времени.

**Ядро ОС** (`os/`) — bare-metal программа на C11 и ассемблере, компилируется без libc.
Загрузчик (`boot.S`) инициализирует стек, устанавливает mtvec и через `mret` остаётся
в M-mode перед вызовом `kernel_main`. Вывод реализован через UART-драйвер (`uart.c`):
запись по MMIO-адресу `0xF000` перехватывается симулятором и выводится в stdout.
Системные вызовы (`ecall`) обрабатываются ОС: `trap.S` сохраняет кадр регистров и передаёт
указатель на него в `trap_handler`; `trap.c` читает `a7` из кадра и диспетчеризует вызов.
Таймерные прерывания реализованы через CLINT-драйвер (`clint.c`): инициализация записывает
`mtimecmp` по MMIO `0xF00C`, симулятор на каждом шаге проверяет `mtime >= mtimecmp` и кидает
`INT_TIMER_M`; обработчик в `kernel_main` сбрасывает таймер через `clint_set_next()`.

```plaintext
┌──────────────────────────────────────────────────────┐
│                     XorOS  (os/)                     │
│                                                      │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────┐   │
│  │ kernel_main  │  │  scheduler   │  │   vmem    │   │
│  └──────────────┘  └──────────────┘  └───────────┘   │
│  ┌────────────────────────────────────────────────┐  │
│  │   trap.S (mtvec) · trap.c (ecall / exceptions) │  │
│  └────────────────────────────────────────────────┘  │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────┐   │
│  │   uart.c     │  │   pipe.c     │  │  kalloc   │   │
│  │ MMIO 0xF000  │  │  ring buf    │  │   bump    │   │
│  └──────────────┘  └──────────────┘  └───────────┘   │
└──────────────────────────────────────────────────────┘
       │ ecall → fireTrap          │ MMIO 0xF000
       │ (EXC_ECALL_U/S/M)         │
       ▼                           ▼
┌──────────────────────────────────────────────────────┐
│              rv32i симулятор  (rv32i/)               │
│                                                      │
│  ┌──────────────────────────────────────────────┐    │
│  │         RVModel<32>   PrivMode M/S/U         │    │
│  │  RV32I + M-ext + A-ext · single-cycle        │    │
│  │  ecall → fireTrap → mtvec                    │    │
│  │  CSR-доступ: проверка привилегий addr[9:8]   │    │
│  │  PTE.U=0: page fault в U-mode                │    │
│  └─────────────────────┬────────────────────────┘    │
│                        │  fetch / load / store       │
│  ┌─────────────────────▼────────────────────────┐    │
│  │      CacheModel<32>   LRU · 64 слов          │    │
│  │      write-through · read-allocate           │    │
│  └─────────────────────┬────────────────────────┘    │
│                        │  miss                       │
│  ┌─────────────────────▼────────────────────────┐    │
│  │   MemoryModel<32>   64 KiB                   │    │
│  │   MMIO: 0xF000 (UART TX → stdout)            │    │
│  │   MMIO: 0xF004 (mtime lo/hi)                 │    │
│  │   MMIO: 0xF00C (mtimecmp lo/hi)              │    │
│  └──────────────────────────────────────────────┘    │
└──────────────────────────────────────────────────────┘
```

### Карта памяти (64 KiB)

```plaintext
Высокие адреса
┌─────────────────────────────────────────────┐
│  MMIO                   0xF000 – 0xFFFF     │  ← volatile uint32_t*
│  UART TX · CLINT mtime/mtimecmp             │     (перехватывается симулятором)
├─────────────────────────────────────────────┤
│  стеки процессов        ↓ растёт вниз       │  ← sp
│  proc_stack_t[MAX_PROCS]                    │
│  [ canary 0xDEADBEEF ] ← младший адрес     │
├─────────────────────────────────────────────┤
│  куча (kalloc bump)     ↑ растёт вверх      │  ← kalloc() → ptr
│  heap[4 × 4 KiB]                            │
├─────────────────────────────────────────────┤
│  BSS (нулевые глобалы)                      │  ← gp ± 2 KiB
│  root_pt · l0_pt · procs[] · chan           │
├─────────────────────────────────────────────┤
│  DATA (инициализированные глобалы)          │  ← gp ± 2 KiB
├─────────────────────────────────────────────┤
│  RODATA                                     │
│  строковые литералы ("hi\n" …)              │
├─────────────────────────────────────────────┤
│  TEXT                   0x0000              │  ← pc · ra
│  _start · kernel_main · trap_entry …        │     mtvec → trap_entry
└─────────────────────────────────────────────┘
Низкие адреса

  Регистры-указатели:
  pc  — текущая инструкция (TEXT)
  ra  — адрес возврата (TEXT)
  sp  — верхушка стека (растёт вниз)
  gp  — середина DATA/BSS (±2 KiB покрывают глобалы)
  satp     → root_pt   → l0_pt  (страничные таблицы Sv32)
  mtvec    → trap_entry          (обработчик прерываний/исключений)
  mepc     → адрес инструкции где случился trap
```

### Архитектурные решения

**Single-cycle, Von Neumann.** Один `step()` — один fetch/decode/execute без конвейера.
Единая память для кода и данных. Состояние процессора полностью определяется значением PC
и регистрового файла — это упрощает отладку ОС по сравнению с реальным железом.

**Flat binary вместо ELF.** ОС компонуется с флагами `-Ttext=0x0 -e _start` в сырой
бинарник без заголовков. Симулятор копирует его в начало виртуальной памяти напрямую —
никакого ELF-загрузчика. Стек располагается по адресу `0x3000` и растёт вниз в пределах 64 KiB.

**ECALL через fireTrap.** При встрече инструкции `ecall` симулятор не вызывает никакой C++
функции — он определяет причину (EXC_ECALL_U, EXC_ECALL_S или EXC_ECALL_M по текущему
`PrivMode`) и передаёт управление на `mtvec`. Обработчик в ОС (`trap.c`) получает указатель
на кадр сохранённых регистров, читает `a7` и диспетчеризует вызов. Это стандартное поведение
RISC-V: симулятор не знает про OS, OS не знает про C++.

**CacheModel как сменный слой памяти.** `RVModel` принимает тип памяти как шаблонный параметр
(`RVModel<XLEN, MemT>`), что позволяет подставить `CacheModel<32>` вместо `MemoryModel<32>`
без изменения ядра симулятора. `CacheModel` реализует LRU-кэш с политикой write-through
и read-allocate. Размер кэша — 64 слова (256 байт). После исполнения симулятор печатает
статистику: количество попаданий, промахов и hit rate.

---

## Состав репозитория

```plaintext
XorOS/
├── rv32i/                  — симулятор RISC-V (C++17): RV32I + M + A расширения
├── os/                     — bare-metal ядро для симулятора (C11 + asm, riscv64-unknown-elf-gcc)
├── scripts/                — вспомогательные скрипты (слияние compile_commands)
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
=== XorOS ===
Hello from XorOS!
hi
kernel: all done

cache: 11317 hits / 3092 misses | 78.5% hit rate
```

### Запуск тестов OS

```bash
cmake --build os/build --target run_tests
```

```plaintext
=== XorOS ===
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

--- kalloc ---
[PASS] kalloc returns non-NULL
[PASS] kalloc page-aligned
[PASS] two allocs differ
[PASS] pages are PAGE_SIZE apart

--- spinlock ---
[PASS] spinlock_init: unlocked
[PASS] spinlock_aquire: locked
[PASS] spinlock_release: unlocked

passed: 27
failed: 0

cache: 464379 hits / 220613 misses | 67.8% hit rate
```

### Запуск тестов симулятора

```bash
./build/rv32i/rv32i_tests
```

---

## Что реализовано

### Симулятор (`rv32i/`)

| Компонент     | Описание                                                              |
|---------------|-----------------------------------------------------------------------|
| RV32I         | Полный набор инструкций: ALU, BRANCH, LOAD/STORE, JAL/JALR            |
| M-extension   | MUL / MULH / MULHSU / MULHU / DIV / DIVU / REM / REMU                 |
| A-extension   | LR.W / SC.W + AMO(SWAP/ADD/XOR/AND/OR/MIN/MAX/MINU/MAXU)              |
| CSR           | CSRRW / CSRRS / CSRRC / CSRRWI / CSRRSI / CSRRCI; проверка привилегий |
| Привилегии    | PrivMode M/S/U; mret восстанавливает из MPP; fireTrap сохраняет в MPP |
| ECALL         | fireTrap(EXC_ECALL_U/S/M) по текущему режиму → mtvec → OS             |
| Sv32 vmem     | Двухуровневая трансляция; PTE.U проверяется в U-mode                  |
| Контекст      | `Context` (callee-saved) и `FullContext` (все 32 рег + PC)            |
| Память        | Плоская, little-endian; LR/SC reservation; MMIO regions               |
| CacheModel    | LRU-кэш (64 слова) поверх MemoryModel; write-through; hit/miss        |
| Дизассемблер  | `Disasm::disassemble()` — DecodedInstr в строку мнемоники             |
| Хуки отладки  | `setStepHook` / `setTrapHook` — колбеки на каждую инструкцию и трап   |
| Dumper        | Дамп регистров, CSR, CLINT, hex-дамп памяти; запись трейса в `.txt`   |

### ОС (`os/`)

| Компонент        | Описание                                                                    |
|------------------|-----------------------------------------------------------------------------|
| `boot.S`         | Инициализация SP, GP, mtvec, mret (MPP=M), вызов `kernel_main`              |
| `ecall.h`        | `sys_putchar` / `sys_exit` через inline asm; обрабатываются OS trap_handler |
| `csr.h`          | Адреса Machine-level CSR, коды mcause (включая CAUSE_ECALL_U/S/M)           |
| `trap.S`         | trap_entry: сохранение всех 32 регистров (*frame, sp через mscratch), mret  |
| `trap.c`         | ecall_handler (a7→uart/halt); диспетчеризация mcause; паника; do_halt       |
| `kalloc.h/c`     | Bump allocator: статический heap[4×4KiB], PAGE_SIZE=4096                    |
| `spinlock.h`     | Header-only spinlock через `amoswap.w.aq` / `amoswap.w.rl` (A-ext)          |
| `process.h`      | PCB: `proc_state_t`, `context_t` (callee-saved), `proc_t`                   |
| `scheduler.h/c`  | Вытесняющий планировщик round-robin: `sched_init/spawn/yield/exit`          |
| `sched_switch.S` | `context_switch` — сохранение/восстановление ra/sp/s0-s11                   |
| `vmem.h/c`       | Sv32 виртуальная память: identity map, satp, sfence.vma (NOP)               |
| `pipe.h/c`       | Кольцевой буфер (16 байт); non-blocking pipe_write/read                     |
| `uart.h/c`       | MMIO UART: uart_putchar/puts → запись в 0xF000                              |
| `clint.h/c`      | Таймер: clint_init(interval) вооружает mtimecmp; clint_set_next() сбрасывает|
| `kernel_main`    | Демо pipe + UART: task_a пишет в pipe, task_b читает через UART; CLINT init |
| Тесты            | 27 тестов: putchar, арифметика, Sv32 vmem, canary, pipe, kalloc, spinlock   |

> Список процессов в планировщике планируется расширить до динамического array-of-slots без malloc на основе [Jigomas/List](https://github.com/Jigomas/List).

---

## Дорожная карта

### Симулятор

- [ ] TLB — кэш VPN→PPN в RVModel; SFENCE.VMA перестаёт быть NOP; добавлять вместе с per-process page tables
- [ ] ELF-загрузчик
- [ ] расширения F / D / C
- [ ] WFI — idle ядро вместо busy-loop; нужна поддержка инструкции в симуляторе

### ОС

- [ ] idle task — pid=0 с wfi; sched_yield без PROC_READY молча возвращается
- [ ] kfree() + свободный список страниц
- [ ] per-process page tables — изоляция процессов; требует kfree
- [ ] U-mode процессы — запускать задачи через mret в PrivMode::U; инфраструктура уже есть
- [ ] расширить таблицу ecall — sys_yield, sys_getpid для U-mode задач
- [ ] блокирующий pipe + BLOCKED-состояние
- [ ] pipe + spinlock — pipe_write/read не thread-safe при вытесняющем планировщике
- [ ] fork + wait
- [ ] exec
- [ ] sbrk(n) syscall
- [ ] COW-форк — ref-count в kalloc, sc.w для fault handler
- [ ] динамический список процессов — array-of-slots без malloc (на основе [Jigomas/List](https://github.com/Jigomas/List))
- [ ] MLFQ — приоритеты в PCB, понижение при использовании кванта, boost раз в период
- [ ] лотерейный планировщик — tickets в PCB, LCG-генератор
- [ ] демо дедлока — два процесса берут два spinlock в разном порядке; watchdog или детектор
- [ ] CoreMark — bare-metal бенчмарк производительности

---

## Литература, без которой реализация проекта была бы невозможной

- Harris & Harris — *Digital Design and Computer Architecture: RISC-V Edition*
- *The RISC-V Instruction Set Manual, Volume I: Unprivileged Architecture*
- *The RISC-V Instruction Set Manual, Volume II: Privileged Architecture*
- Cox, Kaashoek, Morris — *xv6: a simple, Unix-like teaching operating system*
- Arpaci-Dusseau — *Operating Systems: Three Easy Pieces*
- Corbet, Rubini, Kroah-Hartman — *Linux Device Drivers, 3rd Edition*
