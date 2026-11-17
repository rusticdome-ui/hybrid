# Recolha de calibração física — SMK-37 Pro

Este modelo está deliberadamente vazio. **Não contém curvas de fábrica nem valores inventados.** Só preencher depois de ligar o SMK-37 Pro e medir sinais reais.

## Preparação

1. Alimentar o equipamento com fonte estável e deixar aquecer 5 minutos.
2. Ligar o terminal de diagnóstico UART/USB de serviço e registar a versão do firmware, temperatura ambiente, alimentação e identificador da unidade.
3. Confirmar primeiro os 26 canais ADC brutos e o scanner SPI2 sem tocar nos controlos. Cada leitura deve incluir timestamp, índice de conversão, canal ADC, endereço CD4051 e valor bruto 0–1023.
4. Repetir 100 ciclos completos e verificar que não há amostras inválidas nem perda de IRQ. Não guardar uma calibração se houver saltos ou saturação.

## Teclas — 37 curvas

- Medir cada tecla individualmente, sempre com a mesma ferramenta/força controlada.
- Recolher cinco pontos de referência de velocidade, do toque mais leve detetável ao mais forte, com pelo menos 20 repetições por ponto.
- Para o teclado com dois contactos, guardar também o intervalo bruto entre contactos e o estado/ordem dos contactos. A curva não substitui o algoritmo temporal de velocity.
- Calcular a mediana de cada nível; registar em `raw_value`. Os cinco valores de cada tecla têm de ser estritamente crescentes após normalização.

## Pads — 16 curvas

- Medir cada pad nos mesmos cinco níveis de pressão, com força aplicada de forma repetível.
- Registar ADC bruto, canal, mux e mediana de pelo menos 20 amostras por nível.
- Não misturar leituras de pressão com as leituras de ataque/velocidade. Guardar ambas se o diagnóstico as disponibilizar.

## Pedal e controlos lentos

- Varrer o pedal lentamente entre os dois extremos e recolher cinco âncoras.
- Registar também os quatro faders, dois wheels, bateria e referências VBAT/PMU, mas estes canais não devem ser convertidos em curvas de velocidade.
- Repetir o ensaio nos dois sentidos para detetar histerese e ruído.

## Critérios de aceitação

- Valores dentro de 0–1023, sem saturação persistente.
- Cinco âncoras estritamente crescentes por curva.
- Desvio entre repetições documentado; investigar ruído, contactos intermitentes ou mux instável antes de aceitar.
- Verificar monotonicidade, interpolação, latência e repetibilidade com a curva aplicada.
- Guardar registo versionado com CRC32, sem substituir a calibração anterior até passar todos os testes.

## Ficheiros e API

- `SMK37-PRO-CALIBRATION-TEMPLATE.csv`: 37 teclas × 5 âncoras, 16 pads × 5 e pedal × 5.
- `hybrid_ac79_calibration.c`: valida curvas monotónicas, CRC32 e interpolação 0–1023.
- A persistência em memória não volátil e a captura automática ainda dependem do serviço de armazenamento e do diagnóstico de hardware, não incluídos nesta fase.

**Não gravar esta folha vazia no firmware.** Os valores medidos no equipamento devem ser revistos e aprovados antes da ativação.
