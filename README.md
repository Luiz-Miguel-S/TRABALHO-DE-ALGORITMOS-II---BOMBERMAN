# TRABALHO DE ALGORITMOS II — BOMBERMAN
Repositório destinado ao desenvolvimento do trabalho da disciplina Algoritmos e Programação II (1º semestre de 2026), do curso de Ciência da Computação da Universidade do Vale do Itajaí (UNIVALI). Repositório para armazenar todo o desenvolvimento do meu trabalho de algoritmos II - 1° semestre de 2026


# Sobre o projeto
Bomberman é uma clássica série de jogos de estratégia originalmente desenvolvida pela Hudson Soft. O primeiro jogo foi lançado em 1983, e a franquia continua ativa até hoje, com mais de 60 títulos lançados.

Este projeto consiste na implementação de uma versão simplificada do jogo, com foco na aplicação de conceitos de programação, especialmente o uso de sub-rotinas e organização de código.


# Mecânicas do Jogo
1-Jogador
Pode se mover por todas as posições livres do mapa.
Pode posicionar uma bomba na posição atual.
Morre ao:
Entrar na área de explosão de uma bomba;
Colidir com um inimigo.

2-Inimigos
Movem-se periodicamente de forma aleatória.
Podem andar de 1 a 3 posições em uma direção válida até encontrar um obstáculo.
Morrem ao:
Entrar na área de explosão de uma bomba.

3-Bomba
Deve ser posicionada pelo jogador.
Explode após um determinado tempo.
Apenas uma bomba pode estar ativa por vez.
A explosão:
Deve ser visível no jogo;
Elimina jogadores e inimigos próximos;
Destrói paredes frágeis;
Não destrói paredes sólidas.

4-Paredes
O cenário é composto por:
Paredes sólidas (indestrutíveis);
Paredes frágeis (destrutíveis).
Todas impedem a movimentação de jogadores e inimigos.

5-Condições de Jogo
Vitória
O jogador vence ao eliminar todos os inimigos e permanecer vivo.
Derrota
O jogador perde ao:
Ser atingido por uma explosão;
Colidir com um inimigo.

6-Observações
Este trabalho tem como objetivo avaliar:
Organização de código;
Uso de sub-rotinas;
Estruturação lógica do programa.
O código deve ser desenvolvido pensando em facilidade de expansão, permitindo a adição de novas funcionalidades futuramente.
