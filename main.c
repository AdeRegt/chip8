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

typedef struct{
    uint8_t memory[MAX_MEMORY_SIZE];
    uint8_t v[REGISTER_V_SIZE];
    uint16_t I;
    uint8_t sound_timer;
    uint8_t delay_timer;
    uint8_t sp;
    uint16_t pc;
    uint8_t video[SCREEN_HEIGHT*SCREEN_WIDTH];
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
    draw_pixel(x,y,z==1?0x00000000:0xFFFFFFFF);
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
        uint16_t combined_instruction = chip8_fetch_instruction();
        uint16_t instruction = chip8_get_instruction_code_from_instruction(combined_instruction);
        uint16_t argument = chip8_get_argument_from_instruction(combined_instruction);
        if(instruction==1){
            hardware->pc = argument;
        }else if(instruction==2){
            hardware->pc = argument;
            // @TODO :: NEED TO DO SOMETHING WITH THE STACK
        }else if(instruction==3){
            uint8_t vpointer = argument>>8;
            uint8_t vales = argument & 0xFF;
            if(hardware->v[vpointer] == vales){
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
                if( (readvalue & 0b10000000)!=0 ){ chip8_write_to_video(valRegX+0,((valRegY+i)*SCREEN_WIDTH),1); }
                if( (readvalue & 0b01000000)!=0 ){ chip8_write_to_video(valRegX+1,((valRegY+i)*SCREEN_WIDTH),1); }
                if( (readvalue & 0b00100000)!=0 ){ chip8_write_to_video(valRegX+2,((valRegY+i)*SCREEN_WIDTH),1); }
                if( (readvalue & 0b00010000)!=0 ){ chip8_write_to_video(valRegX+3,((valRegY+i)*SCREEN_WIDTH),1); }
                if( (readvalue & 0b00001000)!=0 ){ chip8_write_to_video(valRegX+4,((valRegY+i)*SCREEN_WIDTH),1); }
                if( (readvalue & 0b00000100)!=0 ){ chip8_write_to_video(valRegX+5,((valRegY+i)*SCREEN_WIDTH),1); }
                if( (readvalue & 0b00000010)!=0 ){ chip8_write_to_video(valRegX+6,((valRegY+i)*SCREEN_WIDTH),1); }
                if( (readvalue & 0b00000001)!=0 ){ chip8_write_to_video(valRegX+7,((valRegY+i)*SCREEN_WIDTH),1); }
            }
            hardware->pc += 2;
        }else if(instruction==15&&(argument&0xFF)==0x1E){
            uint8_t vpointer = argument>>8;
            hardware->I += hardware->v[vpointer];
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