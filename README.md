# Array-processing-using-ARM-NEON-and-optimizations

---

Цель: освоить возможности ARM-архитектуры на языке C++ с помощью NEON-
интринсиков, условных операций без ветвлений и баррельного шифтера. Написать
векторную реализацию функции, которая вычисляет сумму модифицированных элементов
массива, максимально используя аппаратные особенности ARM.

## Установка необходимых пакетов

```bash
sudo apt update
sudo apt install g++-arm-linux-gnueabihf gcc-arm-linux-gnueabihf
sudo apt install qemu-user
```

## Сборка и запуск (ARM через QEMU)

Проект собирается кросс-компилятором `arm-linux-gnueabihf-g++`, исполняемый файл
создаётся в корне проекта как `go`.

```bash
make build
make run
```

`make run` выполняет:

```bash
qemu-arm -L /usr/arm-linux-gnueabihf ./go
```

## Что собирается

- `libarray_processing.a` (статическая библиотека)
- `libarray_processing.so` (динамическая библиотека)
- `go` (исполняемый файл)
