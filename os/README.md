# XorOS

Минимальная bare-metal операционная система для rv32i симулятора.

---

## О проекте

Ядро написано на C11 и ассемблере, компилируется без libc в плоский бинарник.
Симулятор загружает его по адресу `0x0` и начинает исполнение с PC=0.
Загрузчик (`boot.S`) инициализирует стек и передаёт управление `kernel_main`.
Единственный интерфейс с внешним миром — два системных вызова через `ecall`:
`sys_putchar` (вывод символа) и `sys_exit` (завершение). Покрыта тестами.

```plaintext
Адресное пространство (64 KiB):
  0x0000  ← _start (код)
  ...
  0x2000  ← начальный SP (стек растёт вниз)
  ...
  0xFFFF  ← конец памяти
```

### Архитектурные решения

**Flat binary вместо ELF.** ОС компонуется с флагами `-Ttext=0x0 -e _start` в сырой
бинарник без заголовков. Симулятор копирует его в начало памяти напрямую — никакого
ELF-загрузчика. Стек ядра в `0x2000`, растёт вниз в пределах 64 KiB.
Стеки процессов встроены в PCB (по 512 байт каждый).

**ECALL вместо MMIO.** Вместо эмуляции UART по фиксированному адресу симулятор
перехватывает `ecall` через C++ callback. ОС не привязана к адресу конкретного
устройства и легко переносима между конфигурациями симулятора.

**Без stdlib.** Флаги `-nostdlib -ffreestanding` — никаких зависимостей от libc.
Всё, что нужно ядру, реализовано вручную или через системные вызовы симулятора.

---

## Структура проекта

```plaintext
os/
├── include/
│   ├── ecall.h       — системные вызовы (sys_putchar, sys_exit)
│   ├── csr.h         — адреса CSR-регистров и коды исключений
│   ├── trap.h        — объявление trap_handler
│   ├── process.h     — PCB: proc_state_t, context_t, proc_t
│   ├── scheduler.h   — API планировщика: sched_init/spawn/yield/exit
│   └── vmem.h        — Sv32 виртуальная память: vmem_map/vmem_enable, флаги PTE
├── src/
│   ├── boot.S          — _start: SP, GP, mtvec, вызов kernel_main
│   ├── kernel_main.c   — демо round-robin: два процесса с sched_yield
│   ├── trap.S          — trap_entry: сохранение регистров, вызов trap_handler, mret
│   ├── trap.c          — диспетчеризация mcause, вывод причины, паника
│   ├── sched_switch.S  — context_switch: сохранение/восстановление callee-saved
│   ├── scheduler.c     — round-robin планировщик, трамплин для новых процессов
│   └── vmem.c          — Sv32 page tables: root_pt/l0_pt, vmem_map, vmem_enable
├── tests/
│   └── test_ecall.c  — 9 тестов: sys_putchar, арифметика (+, -, /, %), Sv32 vmem
├── CMakeLists.txt
└── README.md
```

---

## Требования

| Инструмент              | Версия    |
|-------------------------|-----------|
| CMake                   | ≥ 3.16    |
| riscv64-unknown-elf-gcc | 13.2.0    |
| rv32i_cpu (симулятор)   | из rv32i/ |

Установка кросс-компилятора:

```bash
sudo apt-get install gcc-riscv64-unknown-elf
```

---

## Сборка

OS собирается автоматически из корневого CMake:

```bash
# Из корня репозитория:
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j$(nproc)
```

Результат: `os/build/xoros.bin` и `os/build/xoros_tests.bin` — flat binary.

---

## Запуск

```bash
# Запустить ядро
./rv32i/build/rv32i_cpu os/build/xoros.bin

# Запустить тесты OS через CMake
cmake --build build --target run_tests
```

Ожидаемый вывод ядра:

```plaintext
Hello from XorOS!
task A: hello
task B: hello
task A: bye
task B: bye
kernel: all done
```

Ожидаемый вывод тестов:

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

passed: 9
failed: 0
```

---

## Системные вызовы (`ecall.h`)

Соглашение: номер syscall в `a7`, аргументы в `a0`–`a6`, результат в `a0`.

| Номер (`a7`) | Имя           | Аргумент (`a0`) | Описание               |
|--------------|---------------|-----------------|------------------------|
| 1            | `SYS_PUTCHAR` | символ (char)   | Вывод символа в stdout |
| 10           | `SYS_EXIT`    | —               | Остановка симулятора   |

---

## CSR-регистры (`csr.h`)

Заголовок содержит адреса Machine-level CSR и коды mcause — заготовка для реализации trap-механизма.

| Макрос         | Адрес  | Назначение                        |
|----------------|--------|-----------------------------------|
| `CSR_MSTATUS`  | 0x300  | Статус машинного режима           |
| `CSR_MIE`      | 0x304  | Разрешение прерываний             |
| `CSR_MTVEC`    | 0x305  | Адрес обработчика trap            |
| `CSR_MSCRATCH` | 0x340  | Scratch-регистр для trap-хендлера |
| `CSR_MEPC`     | 0x341  | PC момента исключения             |
| `CSR_MCAUSE`   | 0x342  | Причина trap                      |
| `CSR_MTVAL`    | 0x343  | Доп. информация (адрес/инструкция)|
| `CSR_MIP`      | 0x344  | Ожидающие прерывания              |
| `CSR_SATP`     | 0x180  | Управление трансляцией (Sv32)     |
| `CSR_MHARTID`  | 0xF14  | ID аппаратного потока (всегда 0)  |

---

## Дорожная карта

- [ ] CoreMark — bare-metal бенчмарк производительности
