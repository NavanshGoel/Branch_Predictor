//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include "predictor.h"
#include <math.h>
//
// TODO:Student Information
//
const char *studentName = "CHINMAY SHARMA";
const char *studentID   = "A59023624";
const char *email       = "c1sharma@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//Add your own Branch Predictor data structures here
uint32_t globalHistory;
uint32_t *globalBHT;
uint32_t *localBHT;
uint32_t *localHistoryTable;
uint32_t *choicePredictor;

#define THETA 79
#define PER_HISTORY  34
#define PER_LEN  229
int perceptronHistory[PER_HISTORY];
int perceptronTable[PER_LEN][PER_HISTORY+1];
//

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void initGsharePredictor() {
    int globalSize = (1 << ghistoryBits);
    globalBHT = (uint32_t*)malloc(globalSize * sizeof(uint32_t));
    for (int i=0; i<globalSize; i++) {
        globalBHT[i] = WN;
    }
    globalHistory = 0;
}

void initTournamentPredictor() {
    int localSize = (1 << lhistoryBits);
    int localHistoryTableLength = (1 << pcIndexBits);
    int globalSize = (1 << ghistoryBits);
    int choicePredictorLength = (1 << ghistoryBits);

    localBHT = (uint32_t*)malloc(localSize * sizeof(uint32_t));
    for (int i=0; i<localSize; i++) {
        localBHT[i] = WN;
    }
    
    localHistoryTable = (uint32_t*)malloc(localHistoryTableLength * sizeof(uint32_t));
    for (int i=0; i<localHistoryTableLength; i++) {
        localHistoryTable[i] = 0;
    }

    globalBHT = (uint32_t*)malloc(globalSize * sizeof(uint32_t));
    for (int i=0; i<globalSize; i++) {
        globalBHT[i] = WN;
    }
    globalHistory = 0;

    choicePredictor = (uint32_t*)malloc(choicePredictorLength * sizeof(uint32_t));
    for (int i=0; i<choicePredictorLength; i++) {
        choicePredictor[i] = WN;
    }
}

void initPerceptronPredictor() {
    for (int i=0; i<PER_HISTORY; i++) {
        perceptronHistory[i] = 0;
    }

    for (int i=0; i<PER_LEN; i++) {
        for (int j=0; j<PER_HISTORY+1; j++) {
            perceptronTable[i][j] = 0;
        }
    }
}

void init_predictor() {
    switch(bpType) {
        case GSHARE:
            initGsharePredictor();
            break;
        case TOURNAMENT:
            initTournamentPredictor();
            break;
        case CUSTOM:
            initPerceptronPredictor();
            break;
        default:
            break;
    }
}

uint8_t gsharePredict(uint32_t pc) {
    uint32_t globalSize = (1 << ghistoryBits);
    uint32_t pcLowerBits = pc & (globalSize - 1);
    uint32_t ghistoryLowerBits = globalHistory & (globalSize - 1);
    uint32_t address = pcLowerBits ^ ghistoryLowerBits;

    switch(globalBHT[address]) {
        case 0:
            return NOTTAKEN;
        case 1:
            return NOTTAKEN;
        case 2:
            return TAKEN;
        case 3:
            return TAKEN;
        default:
            printf("Invalid Entry\n");
            return NOTTAKEN;
    }
}

uint8_t tournamentPredict(uint32_t pc) {
    uint32_t globalSize = (1 << ghistoryBits);
    uint32_t pcLowerBits = pc & (globalSize - 1);
    uint32_t ghistoryLowerBits = globalHistory & (globalSize - 1);
    uint32_t address = ghistoryLowerBits;
    uint32_t pcLowLshare = pc & ((1 << pcIndexBits)-1);
    int localBHTAddress = localHistoryTable[pcLowLshare] & ((1 << lhistoryBits)-1);
    uint32_t addressChoice = ghistoryLowerBits;

    if (choicePredictor[addressChoice] == WN || choicePredictor[addressChoice] == SN) {
        switch(localBHT[localBHTAddress]) {
            case WN:
                return NOTTAKEN;
            case SN:
                return NOTTAKEN;
            case WT:
                return TAKEN;
            case ST:
                return TAKEN;
            default:
                printf("Invalid Entry\n");
                return NOTTAKEN;
        }
    } else {
        switch(globalBHT[address]) {
            case WN:
                return NOTTAKEN;
            case SN:
                return NOTTAKEN;
            case WT:
                return TAKEN;
            case ST:
                return TAKEN;
            default:
                printf("Invalid Entry\n");
                return NOTTAKEN;
        }
    }
}

uint8_t preceptronPredict(uint32_t pc) {
    int pcLowerBitsPerceptron = pc % PER_LEN;
    int bias = perceptronTable[pcLowerBitsPerceptron][0];
    int y;

    for (int i=0; i<PER_HISTORY; i++) {
        y = y + perceptronTable[pcLowerBitsPerceptron][i+1] * perceptronHistory[i];
    }
    y = y + bias;

    if (y >= 0) {
        return TAKEN;
    }
    return NOTTAKEN;
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t make_prediction(uint32_t pc) {
    // Make a prediction based on the bpType
    switch (bpType) {
        case GSHARE:
            return gsharePredict(pc);
        case TOURNAMENT:
            return tournamentPredict(pc);
        case CUSTOM:
            return preceptronPredict(pc);
        default:
            break;
    }

    // If there is not a compatable bpType then return NOTTAKEN
    return NOTTAKEN;
}

void trainGsharePredictor(uint32_t pc, uint8_t result) {
    uint32_t globalSize = (1 << ghistoryBits);
    uint32_t pcLowerBits = pc & (globalSize - 1);
    uint32_t ghistoryLowerBits = globalHistory & (globalSize - 1);
    uint32_t address = pcLowerBits ^ ghistoryLowerBits;

    switch(globalBHT[address]) {
        case WN:
            globalBHT[address] = (result == TAKEN) ? WT : SN;
            break;
        case WT:
            globalBHT[address] = (result == TAKEN) ? ST : WN;
            break;
        case ST:
            globalBHT[address] = (result == TAKEN) ? ST : WT;
            break;
        case SN:
            globalBHT[address] = (result == TAKEN) ? WN : SN;
            break;
        default:
            printf("Invalid entry present in branch history table!!");
    }

    if (result == TAKEN) {
        if ((globalHistory << 1)+1 >= globalSize-1) {
            globalHistory = ((globalHistory << 1) + 1) & (globalSize - 1);
        } else {
            globalHistory = (globalHistory << 1) + 1;
        }
    } else {
        if ((globalHistory << 1) >= globalSize-1) {
            globalHistory = (globalHistory << 1) & (globalSize - 1);
        } else {
            globalHistory = (globalHistory << 1);
        }
    }
}

void trainTournamentPredictor(uint32_t pc, uint8_t result) {
    uint32_t globalSize = (1 << ghistoryBits);
    uint32_t pcLowerBits = pc & (globalSize - 1);
    uint32_t ghistoryLowerBits = globalHistory & (globalSize - 1);
    uint32_t globalBHTAddress = ghistoryLowerBits;
    uint32_t pcLowLshare = pc & ((1 << pcIndexBits)-1);
    int localBHTAddress = localHistoryTable[pcLowLshare] & ((1 << lhistoryBits)-1);
    uint32_t addressChoice = ghistoryLowerBits;

    int local = localBHT[localBHTAddress];
    if (local == WN || local == SN) {
        local = NOTTAKEN;
    } else {
        local = TAKEN;
    }

    switch(localBHT[localBHTAddress]) {
        case WN:
            localBHT[localBHTAddress] = (result == TAKEN) ? WT : SN;
            break;
        case WT:
            localBHT[localBHTAddress] = (result == TAKEN) ? ST : WN;
            break;
        case ST:
            localBHT[localBHTAddress] = (result == TAKEN) ? ST : WT;
            break;
        case SN:
            localBHT[localBHTAddress] = (result == TAKEN) ? WN : SN;
            break;
        default:
            printf("Invalid entry present in the branch history table!!");
    }
    localHistoryTable[pcLowLshare] = ((localHistoryTable[pcLowLshare] << 1) | result);

    int global = globalBHT[globalBHTAddress];
    if (global == WN || global == SN) {
        global = NOTTAKEN;
    } else {
        global = TAKEN;
    }

    switch(globalBHT[globalBHTAddress]) {
        case WN:
            globalBHT[globalBHTAddress] = (result == TAKEN) ? WT : SN;
            break;
        case WT:
            globalBHT[globalBHTAddress] = (result == TAKEN) ? ST : WN;
            break;
        case ST:
            globalBHT[globalBHTAddress] = (result == TAKEN) ? ST : WT;
            break;
        case SN:
            globalBHT[globalBHTAddress] = (result == TAKEN) ? WN : SN;
            break;
        default:
            printf("Invalid entry present in the branch history table!!");
    }
    globalHistory = ((globalHistory << 1) | result);

    if (global == result & local != result & choicePredictor[addressChoice] != 3) {
        choicePredictor[addressChoice]++;
    } else if (global != result & local == result & choicePredictor[addressChoice] != 0){
        choicePredictor[addressChoice]--;
    }
}

void trainPerceptronPredictor(uint32_t pc, uint8_t result) {
    int pcLowerBitsPerceptron = pc % PER_LEN;
    int bias = perceptronTable[pcLowerBitsPerceptron][0];
    int y;

    for (int i=0; i<PER_HISTORY; i++) {
        y = y + perceptronTable[pcLowerBitsPerceptron][i+1] * perceptronHistory[i];
    }
    y = y + bias;

    int resultVal = (result == TAKEN) ? 1 : -1;
    if ((y >=0 && !result) || (y < 0 && result) || (abs(y) <= THETA)) {
        perceptronTable[pcLowerBitsPerceptron][0] = perceptronTable[pcLowerBitsPerceptron][0] + 1*resultVal;
        for (int i=0; i<PER_HISTORY; i++) {
            perceptronTable[pcLowerBitsPerceptron][i+1] = perceptronTable[pcLowerBitsPerceptron][i+1] + perceptronHistory[i]*resultVal;
        }
    }

    int history[PER_HISTORY];
    for (int i=0; i<PER_HISTORY; i++) {
        history[i] = perceptronHistory[i];
        if (i == 0) {
            perceptronHistory[0] = history[0];
        } else {
            perceptronHistory[i] = history[i-1];
        }
    }
    perceptronHistory[0] = (result == TAKEN) ? 1 : -1;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void train_predictor(uint32_t pc, uint8_t result) {
    switch(bpType) {
        case GSHARE:
            trainGsharePredictor(pc, result);
            break;
        case TOURNAMENT:
            trainTournamentPredictor(pc, result);
            break;
        case CUSTOM:
            trainPerceptronPredictor(pc, result);
            break;
        default:
            break;
    }
}
