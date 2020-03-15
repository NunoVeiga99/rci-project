# PROJETO RCI #
# Nuno Ramos e Pedro Veiga #

## Programa ##

* O programa abre e cria um servidor TCP e um servidor UDP.
* A interface de utilizador dá as opções á pessoa para escolher o que quer fazer.
* Dependendo da opção os servidores trocam mensagens e fazem coisas (mensagens têm de ser no formato que está no guia).

## Notas para os hackeadores disto ##

* Cada server vai estar a ouvir para ver se recebe alguma mensagem TCP ou UDP, como servidor. 
* E vai ser cliente para mandar mensagem a outros servidores, quando estiver, por exemplo, á procura de uma chave ou a inserir-se no anel.   

Cenas que não fiz:
* Fazer interface e ler informação do input do utilizador com fgets, sscanf e sprintf.
* Informação relacionada com servidor e o seu sucessor deve estar guardada num struct.
* new (should be simple).

