#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#ifdef __SANDEROSUSB
#include <SanderOS.h>
#endif

// reference manual: http://devernay.free.fr/hacks/chip8/C8TECH10.HTM
// emulator: https://chip.netlify.app/emulator/

#define MAX_MEMORY_SIZE 0x1000

#define SCREEN_WIDTH 64
#define SCREEN_HEIGHT 32

#define REGISTER_V_SIZE 16

#ifdef __SANDEROSUSB
#define SCREEN_ENLARGEMENT 10
#endif

typedef struct{
    uint8_t memory[MAX_MEMORY_SIZE];
    uint8_t v[REGISTER_V_SIZE];
    uint16_t I;
    uint8_t sound_timer;
    uint8_t delay_timer;
    uint8_t sp;
    uint16_t stack[15];
    uint16_t pc;
    uint8_t video[SCREEN_HEIGHT*SCREEN_WIDTH];
    uint8_t keyboard;
    uint8_t is_key_pressed;
}CHIP8Hardware;

CHIP8Hardware *hardware;

void chip8_reset_hardware(){
    if(hardware){
        free(hardware);
    }
    hardware = (CHIP8Hardware*) malloc(sizeof(CHIP8Hardware));
    memset(hardware,0,sizeof(CHIP8Hardware));
    hardware->pc = 0x200;

    for(int i = 0 ; i < (SCREEN_HEIGHT*SCREEN_WIDTH) ; i++){
        hardware->video[i] = '-';
    }
}

int chip8_load_file_in_casette(uint8_t *path){
    FILE *bestand = fopen(path,"r");
    if(!bestand){
        return 0;
    }
    long filesize = fseek(bestand,0,SEEK_END);
    #ifndef __SANDEROSUSB
        filesize = ftell(bestand);
    #endif
    if(filesize<1){
        fclose(bestand);
        return 0;
    }

    fseek(bestand,0,SEEK_SET);
    fread((void*)(uint64_t)(hardware->memory + hardware->pc),sizeof(uint8_t),filesize,bestand);

    fclose(bestand);
    return 1;
}

void debug(){
    printf("debug:\n");
    printf("Registers: I:%x PC:%x SP:%x delay:%x sound:%x \n",hardware->I,hardware->pc,hardware->sp,hardware->delay_timer,hardware->sound_timer);
    for(int i = 0 ; i < REGISTER_V_SIZE ; i++){
        printf("V%x:%x ",i,hardware->v[i]);
    }
    printf("\n");
    #ifndef __SANDEROSUSB
    printf("Memory: \n");
    for(int i = 0 ; i < MAX_MEMORY_SIZE ; i++){
        printf("%x ",hardware->memory[i]);
    }
    printf("\n");
    printf("Video: \n");
    for(int y = 0 ; y < SCREEN_HEIGHT ; y++){
        for(int x = 0 ; x < SCREEN_WIDTH ; x++){
            printf("%c",hardware->video[(SCREEN_WIDTH*y)+x]);
        }
        printf("\n");
    }
    printf("\n");
    #endif
}

uint16_t chip8_fetch_instruction(){
    uint16_t num = ((uint16_t*)(hardware->memory + hardware->pc))[0];
    return (num>>8) | (num<<8);
}

uint16_t chip8_get_instruction_code_from_instruction(uint16_t instruction){
    return instruction>>12;
}

uint16_t chip8_get_argument_from_instruction(uint16_t instruction){
    return instruction & 0x0FFF;
}

void chip8_write_to_video(int x,int y,int z){
    int a = (y*SCREEN_WIDTH)+x; 
    if( hardware->video[a]=='X' && z==1 ){
        hardware->v[15] = 1;
    } 
    hardware->video[a] = z==1?'X':'-';
    #ifdef __SANDEROSUSB
    for(int ay = 0 ; ay < SCREEN_ENLARGEMENT ; ay++){
        for(int ax = 0 ; ax < SCREEN_ENLARGEMENT ; ax++){
            draw_pixel((x*SCREEN_ENLARGEMENT)+ax,(y*SCREEN_ENLARGEMENT)+ay,z==1?0x00000000:0xFFFFFFFF);
        }
    }
    #endif
}

int main(int argc,char** argv){
    printf("Welcome to chip-8 emulator!\n");
    chip8_reset_hardware();
    printf("Please define the input file: ");
    uint8_t path[FILENAME_MAX];
    if(argc==2){
        memcpy((uint8_t*)&path,(uint8_t*)argv[1],strlen(argv[1]));
    }else{
        scanf("%s",path);
    }
    printf("Chosen inputfile: %s \n",path);
    if(!chip8_load_file_in_casette(path)){
        exit(EXIT_FAILURE);
    }
    while(1){
        hardware->delay_timer--;
        hardware->keyboard = scan_for_character();
        if(hardware->keyboard=='1'){
            hardware->keyboard = 1;
        }
        if(hardware->keyboard=='2'){
            hardware->keyboard = 2;
        }
        if(hardware->keyboard=='3'){
            hardware->keyboard = 3;
        }
        if(hardware->keyboard=='4'){
            hardware->keyboard = 4;
        }
        if(hardware->keyboard=='5'){
            hardware->keyboard = 5;
        }
        if(hardware->keyboard=='6'){
            hardware->keyboard = 6;
        }
        if(hardware->keyboard=='7'){
            hardware->keyboard = 7;
        }
        if(hardware->keyboard=='8'){
            hardware->keyboard = 8;
        }
        if(hardware->keyboard=='9'){
            hardware->keyboard = 9;
        }
        if(hardware->keyboard=='a'){
            hardware->keyboard = 0xa;
        }
        if(hardware->keyboard=='b'){
            hardware->keyboard = 0xb;
        }
        if(hardware->keyboard=='c'){
            hardware->keyboard = 0xc;
        }
        if(hardware->keyboard=='d'){
            hardware->keyboard = 0xd;
        }
        if(hardware->keyboard=='e'){
            hardware->keyboard = 0xe;
        }
        if(hardware->keyboard=='f'){
            hardware->keyboard = 0xf;
        }
        if(hardware->keyboard=='q'){
            break;
        }
        hardware->is_key_pressed = 1;
        uint16_t combined_instruction = chip8_fetch_instruction();
        uint16_t instruction = chip8_get_instruction_code_from_instruction(combined_instruction);
        uint16_t argument = chip8_get_argument_from_instruction(combined_instruction);
        if(combined_instruction==0x00EE){
            hardware->sp--;
            argument = hardware->stack[hardware->sp];
            hardware->stack[hardware->sp] = 0;
            hardware->pc = argument;
            hardware->pc += 2;
        }else if(instruction==1){
            hardware->pc = argument;
        }else if(instruction==2){
            hardware->stack[hardware->sp++] = hardware->pc;
            hardware->pc = argument;
        }else if(instruction==3){
            uint8_t vpointer = argument>>8;
            uint8_t vales = argument & 0xFF;
            if(hardware->v[vpointer] == vales){
                hardware->pc += 4;
            }else{
                hardware->pc += 2;
            }
        }else if(instruction==4){
            uint8_t vpointer = argument>>8;
            uint8_t vales = argument & 0xFF;
            if(hardware->v[vpointer] != vales){
                hardware->pc += 4;
            }else{
                hardware->pc += 2;
            }
        }else if(instruction==6){
            uint8_t vpointer = argument>>8;
            uint8_t vales = argument & 0xFF;
            hardware->v[vpointer] = vales;
            hardware->pc += 2;
        }else if(instruction==7){
            uint8_t vpointer = argument>>8;
            uint8_t vales = argument & 0xFF;
            hardware->v[vpointer] += vales;
            hardware->pc += 2;
        }else if(instruction==8&&(argument&0xF)==0){
            uint8_t vpointerA = argument>>8;
            uint8_t vpointerB = (argument>>4) & 0xF;
            hardware->v[vpointerA] = hardware->v[vpointerB];
            hardware->pc += 2;
        }else if(instruction==10){
            hardware->I = argument;
            hardware->pc += 2;
        }else if(instruction==13){
            uint8_t regX = (argument & 0xF00)>>8;
            uint8_t regY = (argument & 0x0F0)>>4;
            uint8_t cntN = argument & 0x00F;
            uint8_t valRegX = hardware->v[regX];
            uint8_t valRegY = hardware->v[regY];
            uint16_t addr2read = hardware->I;
            hardware->v[15] = 0;
            for(uint8_t i = 0 ; i < cntN ; i++ ){
                uint8_t readvalue = hardware->memory[addr2read+i];
                if( (readvalue & 0b10000000)!=0 ){ chip8_write_to_video(valRegX+0,valRegY+i,1); }else{ chip8_write_to_video(valRegX+0,valRegY+i,0); }
                if( (readvalue & 0b01000000)!=0 ){ chip8_write_to_video(valRegX+1,valRegY+i,1); }else{ chip8_write_to_video(valRegX+1,valRegY+i,0); }
                if( (readvalue & 0b00100000)!=0 ){ chip8_write_to_video(valRegX+2,valRegY+i,1); }else{ chip8_write_to_video(valRegX+2,valRegY+i,0); }
                if( (readvalue & 0b00010000)!=0 ){ chip8_write_to_video(valRegX+3,valRegY+i,1); }else{ chip8_write_to_video(valRegX+3,valRegY+i,0); }
                if( (readvalue & 0b00001000)!=0 ){ chip8_write_to_video(valRegX+4,valRegY+i,1); }else{ chip8_write_to_video(valRegX+4,valRegY+i,0); }
                if( (readvalue & 0b00000100)!=0 ){ chip8_write_to_video(valRegX+5,valRegY+i,1); }else{ chip8_write_to_video(valRegX+5,valRegY+i,0); }
                if( (readvalue & 0b00000010)!=0 ){ chip8_write_to_video(valRegX+6,valRegY+i,1); }else{ chip8_write_to_video(valRegX+6,valRegY+i,0); }
                if( (readvalue & 0b00000001)!=0 ){ chip8_write_to_video(valRegX+7,valRegY+i,1); }else{ chip8_write_to_video(valRegX+7,valRegY+i,0); }
            }
            hardware->pc += 2;
        }else if(instruction==14&&(argument&0xFF)==0xA1){
            uint8_t vpointerA = argument>>8 & 0xF;
            uint8_t keytocheck= hardware->v[vpointerA];
            if(!(hardware->is_key_pressed==1&&hardware->keyboard==keytocheck)){
                hardware->pc += 4;
            }else{
                hardware->pc += 2;
            }
        }else if(instruction==15&&(argument&0xFF)==0x07){
            uint8_t vpointer = argument>>8;
            hardware->v[vpointer] = hardware->delay_timer;
            hardware->pc += 2;
        }else if(instruction==15&&(argument&0xFF)==0x0A){
            uint8_t vpointer = argument>>8;
            hardware->v[vpointer] = wait_for_character();
            hardware->pc += 2;
        }else if(instruction==15&&(argument&0xFF)==0x15){
            uint8_t vpointer = argument>>8;
            hardware->delay_timer = hardware->v[vpointer];
            hardware->pc += 2;
        }else if(instruction==15&&(argument&0xFF)==0x1E){
            uint8_t vpointer = argument>>8;
            hardware->I += hardware->v[vpointer];
            hardware->pc += 2;
        }else if(instruction==15&&(argument&0xFF)==0x65){
            uint8_t vpointer = argument>>8;
            for(int i = 0 ; i < vpointer ; i++){
                hardware->v[i] = hardware->memory[hardware->I+i];
            }
            hardware->pc += 2;
        }else{
            printf("invalid instruction: %x with argument: %x \n",instruction,argument);
            debug();
            exit(EXIT_FAILURE);
        }
    }
    exit(EXIT_SUCCESS);
    return EXIT_SUCCESS;
}