/* ============================================================================
   BIBLIOTECAS E CONFIGURACOES GERAIS
   ----------------------------------------------------------------------------
   stdio.h   : entrada/saida e manipulacao de arquivos (printf, fgets, fopen).
   stdlib.h  : funcoes gerais da linguagem C.
   string.h  : operacoes com textos (strlen, strcmp, strcpy, strstr).
   ctype.h   : analise/conversao de caracteres (isspace, toupper).

   MAX_LOTES define a capacidade do vetor de lotes.
   MAX_CATEGORIAS define quantas fracoes sao registradas na triagem.
   Os nomes abaixo identificam os arquivos locais de persistencia.
   ============================================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LOTES 100
#define MAX_CATEGORIAS 6
#define ARQUIVO_LOTES "lotes.txt"
#define ARQUIVO_MOVIMENTOS "movimentacoes.txt"

/*
   ECO LOG - Sistema de Controle de Logistica Reversa

   Conceitos de Programacao Estruturada:
   - Estruturas condicionais e de repeticao
   - Funcoes
   - Vetor de estruturas e matriz bidimensional
   - Manipulacao de arquivos

   Estimativa ambiental baseada em fatores por fracao de material de:
   SOUDACHANH, S. et al. Environmental Implications of Reuse: A Case
   Study of Electrical and Electronic Devices in Slovenia. Sustainability,
   v. 18, n. 1, 140, 2026. DOI: 10.3390/su18010140.

   O artigo calcula GWP evitado como massa reciclada x (GWP de material
   virgem - GWP do material reciclado). Os valores sao referenciais,
   derivados de bases e cenarios internacionais, nao fatores validados
   especificamente para o Brasil. Componentes e perifericos sem classificacao
   ficam fora da estimativa ambiental para evitar aplicar fatores indevidos.
*/
/*
   CATEGORIAS DA TRIAGEM
   Cada posicao deste vetor corresponde a uma coluna da matriz
   pesosPorLote. A ultima categoria e usada quando nao e possivel
   classificar um componente/periferico com seguranca.
*/
const char *categorias[MAX_CATEGORIAS] = {
    "Cabos e fios",
    "Plasticos mistos",
    "Aco / ferro",
    "Aluminio",
    "Cobre",
    "Outros componentes e perifericos sem classificacao"
};

/*
   FATORES PARA A ESTIMATIVA AMBIENTAL
   Os dois vetores devem usar a mesma ordem de categorias acima.
   Para cada categoria, o programa calcula:
       fator evitado = fator do material virgem - fator do material reciclado.
   Em seguida multiplica esse fator pela massa estimada daquela fracao.
   A categoria generica tem fatores iguais a zero: ela nao entra no calculo.
   Os fatores sao referencias academicas internacionais, nao fatores oficiais
   validados para a Eco Log ou para todas as cadeias brasileiras de reciclagem.
*/
const float fatorMaterialVirgem[MAX_CATEGORIAS] = {
    6.17f, 3.30f, 5.32f, 8.40f, 5.94f, 0.00f
};
const float fatorMaterialReciclado[MAX_CATEGORIAS] = {
    0.90f, 1.05f, 3.65f, 0.54f, 1.75f, 0.00f
};

/*
   ESTRUTURA DE DADOS DE UM LOTE
   struct permite agrupar em um unico registro as informacoes do cliente,
   recebimento, triagem, transporte e confirmacao pela recicladora.
   Campos int usados como indicadores funcionam como sinalizadores: 0 = nao,
   1 = sim. Os textos possuem tamanhos fixos para simplificar o prototipo.
*/
typedef struct {
    int codigo;
    char cliente[100];
    char dataRecebimento[11];
    char tipoResiduo[60];

    float pesoRecebido;
    int triagemRealizada;

    float pesoEnviado;
    char recicladora[100];
    char placaCaminhao[20];
    char dataEnvio[11];
    int envioRealizado;

    float pesoConfirmado;
    int entregaConfirmada;
} Lote;

/*
   DADOS MANTIDOS EM MEMORIA
   lotes e um vetor de estruturas: cada posicao armazena um lote.
   totalLotes informa quantas posicoes estao ocupadas.
   pesosPorLote e uma matriz bidimensional: linha = lote; coluna = categoria.
   Os dados tambem sao salvos em arquivo para continuarem disponiveis depois
   que o programa for fechado.
*/
Lote lotes[MAX_LOTES];
int totalLotes = 0;
float pesosPorLote[MAX_LOTES][MAX_CATEGORIAS];

/*
   PROTOTIPOS DAS FUNCOES
   Informam ao compilador quais funcoes existem antes que sejam chamadas.
   Isso permite que main e outras funcoes utilizem operacoes definidas adiante.
*/
void lerTexto(const char mensagem[], char destino[], int tamanho);
int lerInteiro(const char mensagem[]);
float lerPeso(const char mensagem[]);
void pausar(void);

int buscarLote(int codigo);
void carregarLotes(void);
void salvarLotes(void);
void registrarMovimentacao(const char evento[]);

/*
   GERA NUMERACAO SEM REUTILIZAR CODIGOS
   Ao excluir um lote, seu cadastro sai de lotes.txt, mas seu evento continua
   no historico. Esta funcao le o historico e encontra o maior codigo ja usado,
   evitando atribuir novamente o mesmo codigo a um lote futuro.
*/
int maiorCodigoNoHistorico(void) {
    FILE *arquivo = fopen(ARQUIVO_MOVIMENTOS, "r");
    char linha[400];
    int maior = 0;

    if (arquivo == NULL) {
        return 0;
    }

    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        char *inicio = strstr(linha, "Lote ");
        int codigo;
        if (inicio != NULL && sscanf(inicio + 5, "%d", &codigo) == 1) {
            if (codigo > maior) {
                maior = codigo;
            }
        }
    }

    fclose(arquivo);
    return maior;
}

void cadastrarLote(void);
void registrarTriagem(void);
void listarLotes(void);
void registrarEnvio(void);
void confirmarEntrega(void);
void consultarPorRecicladora(void);
void consultarMovimentacoes(void);
void emitirCertificado(void);
void gerarRelatorios(void);
void excluirLote(void);
int maiorCodigoNoHistorico(void);

void limparRestoLinha(void);
int textoVazio(const char texto[]);
int dataBasicaValida(const char data[]);

/*
   UTILITARIO DE ENTRADA
   Descarta os caracteres restantes ate a quebra de linha ou fim do arquivo.
   E util quando se precisa limpar sobras da entrada digitada.
*/
void limparRestoLinha(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {
        /* descarta */
    }
}

/*
   LER TEXTO COM SEGURANCA BASICA
   Exibe a pergunta e le uma linha com fgets, respeitando o tamanho maximo
   do destino. Remove a quebra de linha e descarta o excedente quando o texto
   informado e maior que o espaco reservado.
*/
void lerTexto(const char mensagem[], char destino[], int tamanho) {
    size_t comprimento;
    int c;

    if (tamanho <= 0) {
        return;
    }

    printf("%s", mensagem);
    fflush(stdout);

    if (fgets(destino, tamanho, stdin) == NULL) {
        destino[0] = '\0';
        return;
    }

    comprimento = strlen(destino);

    if (comprimento > 0 && destino[comprimento - 1] == '\n') {
        destino[comprimento - 1] = '\0';
    } else {
        /* Se a linha for maior que o buffer, descarta o restante. */
        while ((c = getchar()) != '\n' && c != EOF) {
            /* descarta */
        }
    }
}

/* Retorna 1 se o texto tiver somente espacos/espacos em branco; caso contrario, 0. */
int textoVazio(const char texto[]) {
    int i;

    for (i = 0; texto[i] != '\0'; i++) {
        if (!isspace((unsigned char)texto[i])) {
            return 0;
        }
    }

    return 1;
}

/*
   LER NUMERO INTEIRO
   Repete a pergunta ate que o usuario informe apenas um numero inteiro valido.
   Essa funcao e usada, por exemplo, para opcoes do menu e codigo do lote.
*/
int lerInteiro(const char mensagem[]) {
    char entrada[100];
    int valor;
    char extra;

    while (1) {
        lerTexto(mensagem, entrada, sizeof(entrada));

        if (sscanf(entrada, " %d %c", &valor, &extra) == 1) {
            return valor;
        }

        printf("Entrada invalida. Digite um numero inteiro.\n");
    }
}

/*
   LER PESO
   Le um numero com casas decimais e rejeita entradas negativas ou malformadas.
   Digite o decimal com ponto, por exemplo 125.5. A funcao aceita zero porque
   algumas etapas podem ter categorias sem massa; as funcoes de negocio fazem
   as validacoes adicionais conforme a regra de cada operacao.
*/
float lerPeso(const char mensagem[]) {
    char entrada[100];
    float valor;
    char extra;

    while (1) {
        lerTexto(mensagem, entrada, sizeof(entrada));

        /*
           Em muitos compiladores C, a leitura numerica usa ponto:
           exemplo: 125.5
        */
        if (sscanf(entrada, " %f %c", &valor, &extra) == 1 &&
            valor >= 0.0f) {
            return valor;
        }

        printf("Peso invalido. Digite zero ou um valor positivo (ex.: 125.5).\n");
    }
}

/*
   VALIDACAO BASICA DE DATA
   Verifica o formato DD/MM/AAAA e os intervalos gerais de dia, mes e ano.
   Esta e uma validacao simples: nao confere, por exemplo, os dias especificos
   de cada mes nem anos bissextos.
*/
int dataBasicaValida(const char data[]) {
    int dia, mes, ano;
    char extra;

    if (sscanf(data, "%2d/%2d/%4d%c", &dia, &mes, &ano, &extra) != 3) {
        return 0;
    }

    if (dia < 1 || dia > 31 || mes < 1 || mes > 12 || ano < 2000) {
        return 0;
    }

    return 1;
}

/* Pausa a tela depois de cada operacao para que o usuario leia as mensagens. */
void pausar(void) {
    char entrada[10];

    printf("\nPressione ENTER para voltar ao menu...");
    fflush(stdout);
    fgets(entrada, sizeof(entrada), stdin);
}

/*
   LOCALIZAR LOTE PELO CODIGO
   Percorre os registros cadastrados. Retorna o indice do lote quando encontra;
   retorna -1 se o codigo nao existir. O indice e usado para acessar vetor/matriz.
*/
int buscarLote(int codigo) {
    int i;

    for (i = 0; i < totalLotes; i++) {
        if (lotes[i].codigo == codigo) {
            return i;
        }
    }

    return -1;
}

/*
   CARREGAR DADOS DO ARQUIVO
   Abre lotes.txt para leitura e reconstrui o vetor e a matriz em memoria.
   O codigo entende o formato atual com seis categorias, o formato anterior
   com sete categorias e um formato legado com tres categorias. Quando importa
   registros de versoes antigas, os pesos de placas e itens genericos sao movidos
   para a categoria sem classificacao, que nao recebe fator ambiental.
   Se o arquivo nao existir, a primeira execucao inicia vazia.
*/
void carregarLotes(void) {
    FILE *arquivo = fopen(ARQUIVO_LOTES, "r");
    char linha[1000];

    if (arquivo == NULL) {
        return; /* Arquivo ainda nao existe na primeira execucao. */
    }

    while (fgets(linha, sizeof(linha), arquivo) != NULL &&
           totalLotes < MAX_LOTES) {
        Lote lote;
        float pesos[MAX_CATEGORIAS] = {0};
        float antigasSete[7] = {0};
        float antigosTres[3] = {0};
        int campos, i, separadores = 0;
        char *pos;

        for (pos = linha; *pos != '\0'; pos++) {
            if (*pos == '|') {
                separadores++;
            }
        }

        /*
           FORMATO ATUAL: seis categorias (cabos, plasticos, aco/ferro,
           aluminio, cobre e outros componentes/perifericos sem classificacao).
        */
        campos = sscanf(
            linha,
            "%d|%99[^|]|%10[^|]|%59[^|]|%f|%f|%f|%f|%f|%f|%f|%d|"
            "%f|%99[^|]|%19[^|]|%10[^|]|%d|%f|%d",
            &lote.codigo, lote.cliente, lote.dataRecebimento,
            lote.tipoResiduo, &lote.pesoRecebido,
            &pesos[0], &pesos[1], &pesos[2], &pesos[3], &pesos[4], &pesos[5],
            &lote.triagemRealizada, &lote.pesoEnviado,
            lote.recicladora, lote.placaCaminhao, lote.dataEnvio,
            &lote.envioRealizado, &lote.pesoConfirmado,
            &lote.entregaConfirmada
        );

        if (campos == 19 && separadores == 18) {
            lotes[totalLotes] = lote;
            for (i = 0; i < MAX_CATEGORIAS; i++) {
                pesosPorLote[totalLotes][i] = pesos[i];
            }
            totalLotes++;
            continue;
        }

        /*
           FORMATO INTERMEDIARIO: sete categorias da versao anterior.
           A categoria antiga de placas e a antiga categoria generica sao
           combinadas na categoria atual sem classificacao. Os demais pesos
           sao deslocados para suas categorias de material correspondentes.
        */
        campos = sscanf(
            linha,
            "%d|%99[^|]|%10[^|]|%59[^|]|%f|%f|%f|%f|%f|%f|%f|%f|%d|"
            "%f|%99[^|]|%19[^|]|%10[^|]|%d|%f|%d",
            &lote.codigo, lote.cliente, lote.dataRecebimento,
            lote.tipoResiduo, &lote.pesoRecebido,
            &antigasSete[0], &antigasSete[1], &antigasSete[2],
            &antigasSete[3], &antigasSete[4], &antigasSete[5],
            &antigasSete[6], &lote.triagemRealizada, &lote.pesoEnviado,
            lote.recicladora, lote.placaCaminhao, lote.dataEnvio,
            &lote.envioRealizado, &lote.pesoConfirmado,
            &lote.entregaConfirmada
        );

        if (campos == 20 && separadores == 19) {
            lotes[totalLotes] = lote;
            pesosPorLote[totalLotes][0] = antigasSete[1]; /* cabos e fios */
            pesosPorLote[totalLotes][1] = antigasSete[2]; /* plasticos */
            pesosPorLote[totalLotes][2] = antigasSete[3]; /* aco/ferro */
            pesosPorLote[totalLotes][3] = antigasSete[4]; /* aluminio */
            pesosPorLote[totalLotes][4] = antigasSete[5]; /* cobre */
            pesosPorLote[totalLotes][5] = antigasSete[0] + antigasSete[6];
            totalLotes++;
            continue;
        }

        /*
           FORMATO MAIS ANTIGO: placas eletronicas, cabos e outros componentes.
           O antigo peso de cabos e mapeado para cabos e fios. Os antigos pesos
           de placas e outros componentes vao para a categoria sem classificacao,
           pois esta versao nao calcula emissao evitada para essa categoria.
        */
        campos = sscanf(
            linha,
            "%d|%99[^|]|%10[^|]|%59[^|]|%f|%f|%f|%f|%d|"
            "%f|%99[^|]|%19[^|]|%10[^|]|%d|%f|%d",
            &lote.codigo, lote.cliente, lote.dataRecebimento,
            lote.tipoResiduo, &lote.pesoRecebido,
            &antigosTres[0], &antigosTres[1], &antigosTres[2],
            &lote.triagemRealizada, &lote.pesoEnviado,
            lote.recicladora, lote.placaCaminhao, lote.dataEnvio,
            &lote.envioRealizado, &lote.pesoConfirmado,
            &lote.entregaConfirmada
        );

        if (campos == 16 && separadores == 15) {
            lotes[totalLotes] = lote;
            for (i = 0; i < MAX_CATEGORIAS; i++) {
                pesosPorLote[totalLotes][i] = 0.0f;
            }
            pesosPorLote[totalLotes][0] = antigosTres[1];
            pesosPorLote[totalLotes][5] = antigosTres[0] + antigosTres[2];
            totalLotes++;
        }
    }

    fclose(arquivo);
}
/*
   SALVAR DADOS DOS LOTES
   Reescreve lotes.txt com os registros atuais. Cada linha representa um lote;
   os campos sao separados pelo caractere |. O arquivo guarda os dados gerais,
   os seis pesos da triagem e os estados de envio/entrega.
   Observacao: nomes com o caractere | sao rejeitados no cadastro porque ele
   e reservado como separador do arquivo.
*/
void salvarLotes(void) {
    FILE *arquivo = fopen(ARQUIVO_LOTES, "w");
    int i, j;

    if (arquivo == NULL) {
        printf("Erro: nao foi possivel salvar os lotes.\n");
        return;
    }

    for (i = 0; i < totalLotes; i++) {
        fprintf(arquivo, "%d|%s|%s|%s|%.2f",
                lotes[i].codigo, lotes[i].cliente,
                lotes[i].dataRecebimento, lotes[i].tipoResiduo,
                lotes[i].pesoRecebido);
        for (j = 0; j < MAX_CATEGORIAS; j++) {
            fprintf(arquivo, "|%.2f", pesosPorLote[i][j]);
        }
        fprintf(arquivo, "|%d|%.2f|%s|%s|%s|%d|%.2f|%d\n",
                lotes[i].triagemRealizada, lotes[i].pesoEnviado,
                lotes[i].recicladora, lotes[i].placaCaminhao,
                lotes[i].dataEnvio, lotes[i].envioRealizado,
                lotes[i].pesoConfirmado, lotes[i].entregaConfirmada);
    }

    fclose(arquivo);
}

/*
   REGISTRAR HISTORICO DE MOVIMENTACOES
   Abre movimentacoes.txt no modo "a" (append/acrescimo), portanto adiciona
   uma nova linha sem apagar os eventos anteriores. E chamado apos operacoes
   relevantes como cadastro, triagem, envio, confirmacao, exclusao e certificado.
*/
void registrarMovimentacao(const char evento[]) {
    FILE *arquivo = fopen(ARQUIVO_MOVIMENTOS, "a");

    if (arquivo == NULL) {
        printf("Aviso: nao foi possivel gravar no historico.\n");
        return;
    }

    fprintf(arquivo, "%s\n", evento);
    fclose(arquivo);
}

/*
   1 - CADASTRAR LOTE / CLIENTE
   Coleta os dados iniciais do lote, verifica campos obrigatorios, atribui um
   codigo novo, inicializa pesos e status e salva o lote. Tambem registra o
   recebimento no historico. O lote comeca sem triagem, envio ou entrega.
*/
void cadastrarLote(void) {
    Lote novo;
    int i;
    int maiorCodigo = 0;
    char data[11];

    printf("\n--- CADASTRO DE LOTE / CLIENTE ---\n");

    if (totalLotes >= MAX_LOTES) {
        printf("Limite de %d lotes atingido.\n", MAX_LOTES);
        return;
    }

    lerTexto("Empresa cliente: ", novo.cliente, sizeof(novo.cliente));
    if (textoVazio(novo.cliente) || strchr(novo.cliente, '|') != NULL) {
        printf("Nome invalido ou contem o caractere reservado '|'.\n");
        return;
    }

    lerTexto("Data de recebimento (DD/MM/AAAA): ", data, sizeof(data));
    if (!dataBasicaValida(data)) {
        printf("Data invalida. Use o formato DD/MM/AAAA.\n");
        return;
    }
    strcpy(novo.dataRecebimento, data);

    lerTexto("Tipo de residuo: ", novo.tipoResiduo, sizeof(novo.tipoResiduo));
    if (textoVazio(novo.tipoResiduo) || strchr(novo.tipoResiduo, '|') != NULL) {
        printf("Tipo de residuo invalido.\n");
        return;
    }

    novo.pesoRecebido = lerPeso("Peso recebido em kg: ");
    if (novo.pesoRecebido <= 0.0f) {
        printf("O peso recebido deve ser maior que zero.\n");
        return;
    }

    for (i = 0; i < totalLotes; i++) {
        if (lotes[i].codigo > maiorCodigo) {
            maiorCodigo = lotes[i].codigo;
        }
    }
    {
        int maiorHistorico = maiorCodigoNoHistorico();
        if (maiorHistorico > maiorCodigo) {
            maiorCodigo = maiorHistorico;
        }
    }

    novo.codigo = maiorCodigo + 1;
    novo.triagemRealizada = 0;
    novo.pesoEnviado = 0.0f;
    novo.envioRealizado = 0;
    novo.pesoConfirmado = 0.0f;
    novo.entregaConfirmada = 0;

    strcpy(novo.recicladora, "-");
    strcpy(novo.placaCaminhao, "-");
    strcpy(novo.dataEnvio, "-");

    for (i = 0; i < MAX_CATEGORIAS; i++) {
        pesosPorLote[totalLotes][i] = 0.0f;
    }

    lotes[totalLotes] = novo;
    totalLotes++;

    salvarLotes();

    {
        char evento[250];
        snprintf(evento, sizeof(evento),
                 "Lote %d recebido - Cliente: %s - Peso: %.2f kg",
                 novo.codigo, novo.cliente, novo.pesoRecebido);
        registrarMovimentacao(evento);
    }

    printf("\nLote cadastrado com sucesso!\nCodigo do lote: %d\n", novo.codigo);
}

/*
   2 - REGISTRAR TRIAGEM E PESAGEM
   Localiza um lote ainda nao triado e solicita o peso de cada categoria.
   A soma deve ser maior que zero e nao pode exceder o peso recebido.
   Se a verificacao passar, os pesos sao copiados para a linha correspondente
   da matriz e o status de triagem e atualizado.
*/
void registrarTriagem(void) {
    int codigo, indice, i;
    float pesos[MAX_CATEGORIAS];
    float total = 0.0f;

    printf("\n--- REGISTRAR TRIAGEM E PESAGEM ---\n");
    codigo = lerInteiro("Codigo do lote: ");
    indice = buscarLote(codigo);

    if (indice < 0) {
        printf("Lote nao encontrado.\n");
        return;
    }
    if (lotes[indice].triagemRealizada) {
        printf("A triagem deste lote ja foi registrada.\n");
        return;
    }
    if (lotes[indice].envioRealizado) {
        printf("Nao e possivel registrar triagem apos o envio.\n");
        return;
    }

    printf("Peso recebido: %.2f kg\n", lotes[indice].pesoRecebido);

    for (i = 0; i < MAX_CATEGORIAS; i++) {
        char mensagem[120];
        snprintf(mensagem, sizeof(mensagem), "Peso de %s (kg): ", categorias[i]);
        pesos[i] = lerPeso(mensagem);
        total += pesos[i];
    }

    if (total <= 0.0f || total > lotes[indice].pesoRecebido) {
        printf("Triagem invalida: o total deve ser maior que zero e nao pode superar o peso recebido.\n");
        return;
    }

    for (i = 0; i < MAX_CATEGORIAS; i++) {
        pesosPorLote[indice][i] = pesos[i];
    }

    lotes[indice].triagemRealizada = 1;
    salvarLotes();

    {
        char evento[200];
        snprintf(evento, sizeof(evento),
                 "Triagem registrada - Lote %d - Total separado: %.2f kg",
                 codigo, total);
        registrarMovimentacao(evento);
    }

    printf("Triagem registrada com sucesso. Total separado: %.2f kg\n", total);
}

/*
   3 - LISTAR LOTES
   Percorre o vetor e apresenta os principais dados e o estado de cada lote.
   Se nao houver registros, informa isso ao usuario em vez de listar dados vazios.
*/
void listarLotes(void) {
    int i;

    printf("\n--- LOTES CADASTRADOS ---\n");

    if (totalLotes == 0) {
        printf("Nenhum lote cadastrado.\n");
        return;
    }

    for (i = 0; i < totalLotes; i++) {
        printf("\nCodigo: %d\n", lotes[i].codigo);
        printf("Cliente: %s\n", lotes[i].cliente);
        printf("Data de recebimento: %s\n", lotes[i].dataRecebimento);
        printf("Residuo: %s\n", lotes[i].tipoResiduo);
        printf("Peso recebido: %.2f kg\n", lotes[i].pesoRecebido);
        printf("Triagem: %s\n", lotes[i].triagemRealizada ? "Realizada" : "Pendente");
        printf("Envio: %s\n", lotes[i].envioRealizado ? "Realizado" : "Pendente");
        printf("Entrega: %s\n", lotes[i].entregaConfirmada ? "Confirmada" : "Pendente");
    }
}

/*
   4 - REGISTRAR ENVIO
   Exige um lote existente com triagem concluida e sem envio anterior.
   Soma os pesos da matriz, pede peso efetivamente enviado, recicladora,
   placa do caminhao e data. Depois salva o envio e registra o evento no historico.
*/
void registrarEnvio(void) {
    int codigo, indice, i;
    float pesoSeparado = 0.0f;
    float pesoEnviado;
    char recicladora[100], placa[20], data[11];

    printf("\n--- REGISTRAR ENVIO DE LOTE ---\n");
    codigo = lerInteiro("Codigo do lote: ");
    indice = buscarLote(codigo);

    if (indice < 0) {
        printf("Lote nao encontrado.\n");
        return;
    }
    if (!lotes[indice].triagemRealizada) {
        printf("Registre a triagem antes do envio.\n");
        return;
    }
    if (lotes[indice].envioRealizado) {
        printf("O envio deste lote ja foi registrado.\n");
        return;
    }

    for (i = 0; i < MAX_CATEGORIAS; i++) {
        pesoSeparado += pesosPorLote[indice][i];
    }

    printf("Peso total separado: %.2f kg\n", pesoSeparado);
    pesoEnviado = lerPeso("Peso enviado em kg: ");

    if (pesoEnviado <= 0.0f || pesoEnviado > pesoSeparado) {
        printf("Peso enviado invalido ou superior ao total separado.\n");
        return;
    }

    lerTexto("Nome da recicladora: ", recicladora, sizeof(recicladora));
    if (textoVazio(recicladora) || strchr(recicladora, '|') != NULL) {
        printf("Nome da recicladora invalido.\n");
        return;
    }

    lerTexto("Placa do caminhao: ", placa, sizeof(placa));
    if (textoVazio(placa) || strchr(placa, '|') != NULL) {
        printf("Placa invalida.\n");
        return;
    }

    lerTexto("Data do envio (DD/MM/AAAA): ", data, sizeof(data));
    if (!dataBasicaValida(data)) {
        printf("Data invalida. Use o formato DD/MM/AAAA.\n");
        return;
    }

    lotes[indice].pesoEnviado = pesoEnviado;
    strcpy(lotes[indice].recicladora, recicladora);
    strcpy(lotes[indice].placaCaminhao, placa);
    strcpy(lotes[indice].dataEnvio, data);
    lotes[indice].envioRealizado = 1;

    salvarLotes();

    {
        char evento[250];
        snprintf(evento, sizeof(evento),
                 "Envio - Lote %d - Recicladora: %s - Caminhao: %s - Peso: %.2f kg",
                 codigo, recicladora, placa, pesoEnviado);
        registrarMovimentacao(evento);
    }

    printf("Envio registrado com sucesso.\n");
}

/*
   5 - CONFIRMAR ENTREGA
   Registra o peso informado na confirmacao da recicladora. So permite a
   operacao se houver envio anterior e ainda nao existir confirmacao.
   Se o peso recebido for menor que o enviado, apresenta a diferenca para revisao.
*/
void confirmarEntrega(void) {
    int codigo, indice;
    float peso;

    printf("\n--- CONFIRMAR ENTREGA ---\n");
    codigo = lerInteiro("Codigo do lote: ");
    indice = buscarLote(codigo);

    if (indice < 0) {
        printf("Lote nao encontrado.\n");
        return;
    }
    if (!lotes[indice].envioRealizado) {
        printf("Nao existe envio registrado para este lote.\n");
        return;
    }
    if (lotes[indice].entregaConfirmada) {
        printf("A entrega deste lote ja foi confirmada.\n");
        return;
    }

    printf("Recicladora: %s\n", lotes[indice].recicladora);
    printf("Peso enviado: %.2f kg\n", lotes[indice].pesoEnviado);

    peso = lerPeso("Peso confirmado pela recicladora (kg): ");

    if (peso <= 0.0f || peso > lotes[indice].pesoEnviado) {
        printf("Peso confirmado invalido. Verifique os registros.\n");
        return;
    }

    lotes[indice].pesoConfirmado = peso;
    lotes[indice].entregaConfirmada = 1;
    salvarLotes();

    {
        char evento[200];
        snprintf(evento, sizeof(evento),
                 "Entrega confirmada - Lote %d - Peso: %.2f kg",
                 codigo, peso);
        registrarMovimentacao(evento);
    }

    printf("Entrega confirmada com sucesso.\n");
    if (peso < lotes[indice].pesoEnviado) {
        printf("Atencao: diferenca de %.2f kg entre envio e recebimento.\n",
               lotes[indice].pesoEnviado - peso);
    }
}

/*
   6 - CONSULTAR POR RECICLADORA
   Solicita o nome da recicladora e percorre os lotes procurando correspondencias
   exatas. Para cada lote encontrado, mostra cliente, veiculo e pesos enviados/
   confirmados. A busca de texto diferencia maiusculas/minusculas nesta versao.
*/
void consultarPorRecicladora(void) {
    char nome[100];
    int i, encontrados = 0;

    printf("\n--- CONSULTAR LOTES POR RECICLADORA ---\n");
    lerTexto("Nome da recicladora: ", nome, sizeof(nome));

    for (i = 0; i < totalLotes; i++) {
        if (strcmp(lotes[i].recicladora, nome) == 0) {
            printf("\nLote: %d\n", lotes[i].codigo);
            printf("Cliente: %s\n", lotes[i].cliente);
            printf("Caminhao: %s\n", lotes[i].placaCaminhao);
            printf("Peso enviado: %.2f kg\n", lotes[i].pesoEnviado);
            printf("Peso confirmado: %.2f kg\n", lotes[i].pesoConfirmado);
            encontrados++;
        }
    }

    if (encontrados == 0) {
        printf("Nenhum lote encontrado para essa recicladora.\n");
    }
}

/*
   7 - CONSULTAR HISTORICO
   Abre movimentacoes.txt para leitura e mostra cada linha registrada.
   O historico e mantido mesmo quando um lote e excluido do cadastro, permitindo
   conservar o rastro do evento de exclusao e das operacoes anteriores.
*/
void consultarMovimentacoes(void) {
    FILE *arquivo = fopen(ARQUIVO_MOVIMENTOS, "r");
    char linha[300];

    printf("\n--- HISTORICO DE MOVIMENTACOES ---\n");

    if (arquivo == NULL) {
        printf("Ainda nao existem movimentacoes registradas.\n");
        return;
    }

    while (fgets(linha, sizeof(linha), arquivo) != NULL) {
        printf("%s", linha);
    }

    fclose(arquivo);
}

/*
   10 - EXCLUIR LOTE
   Mostra os dados principais e solicita confirmacao explicita (S/N).
   Se confirmado, remove o lote do vetor, desloca os registros seguintes e
   atualiza lotes.txt. Tambem tenta remover certificado_<codigo>.txt e registra
   a exclusao no historico. O historico antigo nao e apagado.
   Esta e uma exclusao definitiva do cadastro local; deve ser usada com cuidado.
*/
void excluirLote(void) {
    int codigo, indice, i, j;
    char confirmacao[20];
    char cliente[100];
    char nomeCertificado[80];

    printf("\n--- EXCLUIR LOTE ---\n");
    if (totalLotes == 0) {
        printf("Nao ha lotes cadastrados para excluir.\n");
        return;
    }

    codigo = lerInteiro("Codigo do lote que deseja excluir: ");
    indice = buscarLote(codigo);
    if (indice < 0) {
        printf("Lote nao encontrado.\n");
        return;
    }

    printf("Lote: %d | Cliente: %s | Peso recebido: %.2f kg\n",
           lotes[indice].codigo, lotes[indice].cliente,
           lotes[indice].pesoRecebido);
    printf("Atencao: a exclusao remove o lote do cadastro e o certificado associado,\n");
    printf("mas nao apaga o historico de movimentacoes ja registrado.\n");
    lerTexto("Confirma a exclusao? Digite S para sim ou N para cancelar: ",
             confirmacao, sizeof(confirmacao));

    if (toupper((unsigned char)confirmacao[0]) != 'S') {
        printf("Exclusao cancelada.\n");
        return;
    }

    strcpy(cliente, lotes[indice].cliente);
    snprintf(nomeCertificado, sizeof(nomeCertificado), "certificado_%d.txt", codigo);

    for (i = indice; i < totalLotes - 1; i++) {
        lotes[i] = lotes[i + 1];
        for (j = 0; j < MAX_CATEGORIAS; j++) {
            pesosPorLote[i][j] = pesosPorLote[i + 1][j];
        }
    }

    totalLotes--;
    for (j = 0; j < MAX_CATEGORIAS; j++) {
        pesosPorLote[totalLotes][j] = 0.0f;
    }

    salvarLotes();
    if (remove(nomeCertificado) == 0) {
        printf("Certificado associado removido: %s\n", nomeCertificado);
    }

    {
        char evento[250];
        snprintf(evento, sizeof(evento),
                 "Lote %d excluido do cadastro - Cliente: %s - historico anterior preservado",
                 codigo, cliente);
        registrarMovimentacao(evento);
    }

    printf("Lote %d excluido do cadastro.\n", codigo);
}

/*
   8 - EMITIR CERTIFICADO AMBIENTAL
   Exige lote existente, triagem registrada e entrega confirmada. Como o prototipo
   nao registra a composicao exata do lote dentro do caminhao, estima a massa de
   cada categoria proporcionalmente aos pesos anotados na triagem.

   Para cada categoria com fator disponivel:
       fator evitado = fator virgem - fator reciclado
       emissao evitada += massa estimada da categoria * fator evitado
   A categoria generica de componentes/perifericos nao tem fator especifico e nao
   contribui para a estimativa. O resultado e uma estimativa academica, nao medicao direta.
   O documento e salvo em certificado_<codigo>.txt e o evento entra no historico.
*/
void emitirCertificado(void) {
    int codigo, indice, i;
    char nomeArquivo[80];
    FILE *arquivo;
    float totalTriado = 0.0f;
    float proporcaoConfirmada;
    float pesoCalculado = 0.0f;
    float pesoSemFator = 0.0f;
    float emissaoEvitada = 0.0f;

    printf("\n--- EMITIR CERTIFICADO AMBIENTAL ---\n");
    codigo = lerInteiro("Codigo do lote: ");
    indice = buscarLote(codigo);

    if (indice < 0) {
        printf("Lote nao encontrado.\n");
        return;
    }
    if (!lotes[indice].triagemRealizada) {
        printf("Certificado bloqueado: a triagem ainda nao foi registrada.\n");
        return;
    }
    if (!lotes[indice].entregaConfirmada) {
        printf("Certificado bloqueado: a entrega ainda nao foi confirmada.\n");
        return;
    }

    for (i = 0; i < MAX_CATEGORIAS; i++) {
        totalTriado += pesosPorLote[indice][i];
    }
    if (totalTriado <= 0.0f) {
        printf("Certificado bloqueado: nao ha pesos de triagem para calcular.\n");
        return;
    }

    /*
       HIPOTESE DE COMPOSICAO DA CARGA:
       O prototipo guarda a composicao triada, mas nao informa exatamente quanto
       de cada categoria foi colocado no caminhao. Por isso, distribui o peso
       confirmado proporcionalmente aos pesos das categorias na triagem.
       Exemplo: se 20% da triagem era plastico, estima que 20% do peso confirmado
       corresponde a plastico. Essa aproximacao e informada no certificado.
    */
    /*
       Proporcao da entrega em relacao ao total da triagem. Multiplicada pelo
       peso de cada categoria, estima a massa dessa categoria na entrega.
    */
    proporcaoConfirmada = lotes[indice].pesoConfirmado / totalTriado;
    for (i = 0; i < MAX_CATEGORIAS; i++) {
        float pesoFracaoConfirmada = pesosPorLote[indice][i] * proporcaoConfirmada;
        float fatorEvitado = fatorMaterialVirgem[i] - fatorMaterialReciclado[i];

        /* Apenas categorias com diferenca positiva entre os fatores entram no calculo. */
        if (fatorEvitado > 0.0f) {
            emissaoEvitada += pesoFracaoConfirmada * fatorEvitado;
            pesoCalculado += pesoFracaoConfirmada;
        } else {
            pesoSemFator += pesoFracaoConfirmada;
        }
    }

    /* O certificado e um arquivo de texto individual identificado pelo codigo do lote. */
    snprintf(nomeArquivo, sizeof(nomeArquivo), "certificado_%d.txt", codigo);
    arquivo = fopen(nomeArquivo, "w");
    if (arquivo == NULL) {
        printf("Erro ao criar o arquivo do certificado.\n");
        return;
    }

    fprintf(arquivo, "============================================================\n");
    fprintf(arquivo, "                         ECO LOG\n");
    fprintf(arquivo, "                  CERTIFICADO AMBIENTAL\n");
    fprintf(arquivo, "============================================================\n\n");
    fprintf(arquivo,
            "A empresa %s encaminhou %.2f kg de residuos para destinacao a reciclagem.\n",
            lotes[indice].cliente, lotes[indice].pesoConfirmado);
    fprintf(arquivo,
            "Considerando a hipotese de reciclagem das fracoes recebidas e os fatores de referencia adotados,\n"
            "estima-se um potencial de emissoes evitadas de %.2f kg de CO2e,\n"
            "em emissoes de gases de efeito estufa.\n\n",
            emissaoEvitada);
    fprintf(arquivo, "Codigo do lote: %d\n", codigo);
    fprintf(arquivo, "Tipo de residuo informado: %s\n", lotes[indice].tipoResiduo);
    fprintf(arquivo, "Recicladora: %s\n", lotes[indice].recicladora);
    fprintf(arquivo, "Peso confirmado pela recicladora: %.2f kg\n",
            lotes[indice].pesoConfirmado);
    fprintf(arquivo, "Massa estimada considerada no calculo ambiental: %.2f kg\n",
            pesoCalculado);
    fprintf(arquivo, "Massa sem fator especifico aplicado: %.2f kg\n\n",
            pesoSemFator);

    /* Registra a metodologia e as limitacoes para que o numero nao pareca medicao direta. */
    fprintf(arquivo, "METODOLOGIA SIMPLIFICADA\n");
    fprintf(arquivo,
            "Emissoes evitadas = massa da fracao destinada a reciclagem x (GWP do material virgem - GWP do material reciclado).\n");
    fprintf(arquivo,
            "A composicao da entrega confirmada foi estimada proporcionalmente aos pesos registrados na triagem.\n");
    fprintf(arquivo,
            "As fracoes sem fator de referencia especifico nao foram contabilizadas na estimativa.\n");
    fprintf(arquivo,
            "Fatores de referencia: Soudachanh et al. (2026), Sustainability, 18(1), 140. DOI: 10.3390/su18010140.\n");
    fprintf(arquivo,
            "Os fatores sao baseados em fontes e cenarios internacionais e nao foram validados especificamente para o Brasil.\n");
    fprintf(arquivo,
            "Componentes e perifericos sem classificacao de material ficaram fora do calculo ambiental.\n");
    fprintf(arquivo,
            "Este documento apresenta uma estimativa academica, nao uma medicao direta nem uma certificacao ambiental verificada.\n");

    fclose(arquivo);

    {
        char evento[200];
        snprintf(evento, sizeof(evento),
                 "Certificado gerado - Lote %d - Arquivo: %s",
                 codigo, nomeArquivo);
        registrarMovimentacao(evento);
    }

    printf("Certificado gerado com sucesso: %s\n", nomeArquivo);
    printf("Estimativa calculada: %.2f kg CO2e\n", emissaoEvitada);
}

/*
   9 - RELATORIO OPERACIONAL
   Percorre todos os lotes para somar peso recebido, enviado e confirmado.
   Tambem conta os lotes com entrega confirmada e os que ainda estao pendentes.
   Nesta versao, o resumo e exibido na tela; nao e criado um arquivo de relatorio.
*/
void gerarRelatorios(void) {
    int i;
    int entregues = 0;
    int pendentes = 0;
    float totalRecebido = 0.0f;
    float totalEnviado = 0.0f;
    float totalConfirmado = 0.0f;

    printf("\n--- RELATORIOS OPERACIONAIS ---\n");

    /* O loop agrega indicadores de cada lote para construir o resumo. */
    for (i = 0; i < totalLotes; i++) {
        totalRecebido += lotes[i].pesoRecebido;
        totalEnviado += lotes[i].pesoEnviado;
        totalConfirmado += lotes[i].pesoConfirmado;

        if (lotes[i].entregaConfirmada) {
            entregues++;
        } else {
            pendentes++;
        }
    }

    printf("Quantidade de lotes: %d\n", totalLotes);
    printf("Peso total recebido: %.2f kg\n", totalRecebido);
    printf("Peso total enviado: %.2f kg\n", totalEnviado);
    printf("Peso total confirmado: %.2f kg\n", totalConfirmado);
    printf("Lotes com entrega confirmada: %d\n", entregues);
    printf("Lotes pendentes de confirmacao: %d\n", pendentes);
}

/*
   FUNCAO PRINCIPAL - CONTROLE DO PROGRAMA
   1. Carrega os registros salvos na inicializacao.
   2. Exibe o menu em um loop do...while.
   3. Usa switch para chamar a funcao da opcao escolhida.
   4. Encerra apenas quando o usuario digita 0 e salva os lotes novamente.
*/
int main(void) {
    int opcao;

    carregarLotes();

    /* do...while garante que o menu seja apresentado pelo menos uma vez. */
    do {
        printf("\n========================================\n");
        printf("                 ECO LOG\n");
        printf(" Sistema de Controle de Logistica Reversa\n");
        printf("========================================\n");
        printf("1 - Cadastrar lote / cliente\n");
        printf("2 - Registrar triagem e pesagem\n");
        printf("3 - Listar lotes cadastrados\n");
        printf("4 - Registrar envio de lote\n");
        printf("5 - Confirmar entrega\n");
        printf("6 - Consultar lotes por recicladora\n");
        printf("7 - Consultar movimentacoes\n");
        printf("8 - Emitir certificado ambiental\n");
        printf("9 - Relatorios operacionais\n");
        printf("10 - Excluir lote\n");
        printf("0 - Sair\n");
        printf("========================================\n");

        opcao = lerInteiro("Escolha uma opcao: ");

        /* switch direciona a escolha do usuario para a funcionalidade correspondente. */
        switch (opcao) {
            case 1: cadastrarLote(); break;
            case 2: registrarTriagem(); break;
            case 3: listarLotes(); break;
            case 4: registrarEnvio(); break;
            case 5: confirmarEntrega(); break;
            case 6: consultarPorRecicladora(); break;
            case 7: consultarMovimentacoes(); break;
            case 8: emitirCertificado(); break;
            case 9: gerarRelatorios(); break;
            case 10: excluirLote(); break;
            case 0: printf("\nEncerrando o sistema Eco Log...\n"); break;
            default: printf("\nOpcao invalida. Tente novamente.\n");
        }

        if (opcao != 0) {
            pausar();
        }

    } while (opcao != 0);

    /* Salva os dados antes de encerrar para preservar as alteracoes da sessao. */
    salvarLotes();
    return 0; /* Codigo 0 indica encerramento normal do programa. */
}

