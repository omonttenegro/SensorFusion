# SensorFusion

Projeto em C para simulação de um sistema de estimação distribuída com múltiplos processos, no âmbito de Sistemas Operativos.

O sistema é composto por:

- **Main**: processo principal, responsável por inicializar estruturas, criar processos e interagir com o utilizador;
- **Sensors**: geram medições aleatórias com ruído;
- **Controllers**: recebem e validam medições;
- **Servers**: recebem medições validadas e calculam estimativas por ciclo.

A comunicação entre processos é feita através de **memória partilhada** e de **buffers partilhados**, usando `fork()`, `shm_open()`, `mmap()` e sincronização por espera ativa simples.

---

## Objetivo

O objetivo do projeto é simular um sistema onde, em cada ciclo de medição:

1. o processo principal cria um pedido de medição;
2. todos os sensores geram uma medição para esse pedido;
3. cada controller recebe a medição associada ao seu sensor e valida-a;
4. os servers recebem as medições validadas e calculam uma estimativa final para o ciclo.

Se num dado ciclo existirem valores válidos, a estimativa corresponde à média desses valores. Se todas as medições forem inválidas, o resultado desse ciclo é `NULL`. :contentReference[oaicite:2]{index=2}

---

## Arquitetura do sistema

### 1. Main
O processo principal:

- lê os argumentos da linha de comandos;
- cria memória dinâmica e memória partilhada;
- cria os processos filhos;
- aceita comandos do utilizador;
- termina o sistema de forma controlada e apresenta estatísticas finais. :contentReference[oaicite:3]{index=3}

### 2. Sensor
Cada sensor:

- espera por um pedido de medição com o identificador esperado;
- gera uma medição pseudoaleatória com ruído gaussiano;
- atualiza os contadores globais;
- envia a medição para o buffer `sensor -> controller`. :contentReference[oaicite:4]{index=4} :contentReference[oaicite:5]{index=5}

### 3. Controller
Cada controller:

- lê a medição produzida pelo sensor correspondente;
- associa o seu identificador ao registo;
- valida a medição;
- marca-a como `VALID` ou `INVALID`;
- envia o resultado para o buffer `controller -> server`. :contentReference[oaicite:6]{index=6}

### 4. Server
Cada server:

- lê, para cada ciclo, uma medição de cada controller;
- acumula os valores válidos;
- calcula a média final;
- imprime a estimativa no `stdout`. :contentReference[oaicite:7]{index=7} :contentReference[oaicite:8]{index=8}

> Nota: nesta primeira fase, se existirem vários servers, pode acontecer mais do que um tentar escrever no `stdout` ao mesmo tempo. Esse problema de sincronização é esperado e fica para resolução posterior.

---

## Estruturas principais

A estrutura central do projeto é `MeasurementInfo`, que representa uma medição ou pedido de medição. Inclui:

- estado (`REQUEST`, `MEASURED`, `VALID`, `INVALID`);
- identificador da medição (`m_id`);
- identificador do sensor;
- identificador do controller;
- valor medido;
- contadores auxiliares para controlo de leitura por sensores e servers. :contentReference[oaicite:9]{index=9}

Também existem duas estruturas principais para comunicação:

- **circular buffer** para `main -> sensors`;
- **random-access buffers** para `sensors -> controllers` e `controllers -> servers`. :contentReference[oaicite:10]{index=10}

---

## Organização dos ficheiros

```text
.
├── bin/
├── obj/
├── src/
│   ├── main.c
│   ├── process.c
│   ├── memory.c
│   ├── sensor.c
│   ├── controller.c
│   ├── server.c
│   └── random_measurement.c
├── inc/
│   ├── main.h
│   ├── process.h
│   ├── memory.h
│   ├── sensor.h
│   ├── controller.h
│   ├── server.h
│   └── random_measurement.h
└── Makefile
