
# H.A.N.D. – Human Assistant Natural Device

## Descrição do Projeto

A H.A.N.D. (Human Assistant Natural Device) é uma mão robótica desenvolvida no âmbito da Prova de Aptidão Profissional (PAP) do Curso Técnico de Gestão de Equipamentos Informáticos da Escola Técnica e Profissional do Ribatejo.

O projeto tem como objetivo demonstrar a integração entre eletrónica, programação e robótica através da criação de uma maquete funcional inspirada no movimento da mão humana. A H.A.N.D. permite controlar individualmente os dedos através de servomotores, utilizando uma interface web acessível a partir de qualquer dispositivo ligado à rede criada pelo sistema.

Embora não seja uma prótese médica, o projeto baseia-se nos princípios utilizados em mãos robóticas modernas, servindo como ferramenta de aprendizagem e demonstração tecnológica.

---

## Objetivos

- Desenvolver uma mão robótica funcional.
- Demonstrar a integração entre hardware e software.
- Aplicar conhecimentos de programação, eletrónica e redes.
- Criar uma interface web intuitiva para controlo remoto.
- Simular movimentos básicos da mão humana.

---

## Tecnologias Utilizadas

### Hardware

- Arduino Nano ESP32
- Servomotores
- Servo Motor Driver Board
- Protoboard
- Cabos Jumper
- Fonte de alimentação externa
- Estrutura mecânica da mão robótica

### Software

- Arduino IDE
- Linguagem C/C++
- HTML
- CSS
- JavaScript
- Wi-Fi Access Point (ESP32)

---

## Funcionamento

O Arduino Nano ESP32 cria uma rede Wi-Fi local à qual o utilizador se pode ligar através de um computador ou smartphone.

Após a ligação à rede, basta abrir um navegador e aceder ao endereço:

```text
192.168.4.1
```

A interface web permite:

- Controlar cada dedo individualmente;
- Executar gestos pré-definidos;
- Ajustar posições através de sliders;
- Enviar comandos em tempo real para os servomotores.

Os comandos são processados pelo ESP32 e convertidos em sinais PWM que controlam os movimentos da mão robótica.

---

## Instruções de Utilização

### 1. Verificar as ligações

Confirmar que todos os componentes estão corretamente ligados:

- Arduino Nano ESP32
- Servomotores
- Driver Board
- Fonte de alimentação

### 2. Ligar o sistema

Ligar a alimentação da H.A.N.D.

### 3. Ligar à rede Wi-Fi

Procurar a rede criada pelo ESP32 e estabelecer ligação.

### 4. Abrir a interface

No navegador, aceder a:

```text
http://192.168.4.1
```

### 5. Controlar a H.A.N.D.

Utilizar os botões e sliders disponíveis para movimentar os dedos e executar gestos.

### 6. Encerrar o sistema

Quando terminar a utilização, desligar a alimentação da H.A.N.D.

---

## Estrutura do Projeto

```text
H.A.N.D.
│
├── Código Arduino
├── Interface Web
├── Configuração Wi-Fi
├── Controlo dos Servomotores
└── Documentação
```

---

## Resultados Obtidos

- Controlo individual dos dedos.
- Interface web funcional.
- Comunicação sem fios através do ESP32.
- Movimentos estáveis após calibração.
- Demonstração prática de conceitos de robótica e sistemas embebidos.

---

## Melhorias Futuras

- Desenvolvimento de um braço robótico completo.
- Integração de sensores.
- Implementação de controlo por voz.
- Adição de câmara para visão computacional.
- Introdução de feedback automático dos movimentos.

---

## Autor

**José Vassalo**

Prova de Aptidão Profissional (PAP)  
Curso Técnico de Gestão de Equipamentos Informáticos  
Escola Técnica e Profissional do Ribatejo  
Ano Letivo 2025/2026
