# Jardim das Runas

Trabalho desenvolvido para o Grau B da disciplina de Processamento Gráfico da UNISINOS.

## 👥 Integrantes do Grupo

- Gustavo Saraiva Lampert (trabalho feito de forma individual)

## 🚀 Como executar

### Pré-requisitos

- Visual Studio 2022 Build Tools (MSVC)
- CMake
- VS Code com as extensões C/C++ e CMake Tools

### Compilação

1. Abra a pasta do projeto no VS Code
2. Selecione o kit **Visual Studio Build Tools 2022 - amd64**
3. Pressione **F7** para compilar
4. Pressione **Shift + F5** para executar

## 💡 Sobre o Programa

Jardim das Runas é um protótipo de jogo com mapa isométrico do tipo diamond desenvolvido em OpenGL. O jogador controla um espírito que deve ativar todas as runas espalhadas e alcançar o portal de saída enquanto desvia da corrupção espalhada pelo mapa.

## 🎮 Controles

| Tecla | Ação |
|---------|---------|
| W + D / ↑ + → | Norte |
| A + S / ← + ↓ | Sul |
| W + A / ↑ + ← | Oeste |
| D + S / → + ↓ | Leste |
| W / ↑ | Noroeste |
| S / ↓ | Sudeste |
| A / ← | Sudoeste |
| D / → | Nordeste |
| R | Reiniciar jogo |
| ESC | Sair |

## 📌 Regras do Jogo

- Colete as 10 runas espalhadas pelo mapa.
- Após coletar todas as runas, o portal será ativado.
- Alcance o portal para vencer.
- Pisar em um tile de corrupção resulta em derrota.
- A partida pode ser reiniciada a qualquer momento pressionando **R**.

## 📂 Arquivos necessários

O projeto utiliza os seguintes recursos:

- `map.txt`
- `tilesetIso.png`
- `portal.png`
- `runa.png`
- `spirit.png`

Além das bibliotecas utilizadas na disciplina:

- GLAD
- GLFW
- GLM
- stb_image

## 📚 Referências

- Template de README disponibilizado pelo professor.
- Material e exemplos da disciplina de Processamento Gráfico.