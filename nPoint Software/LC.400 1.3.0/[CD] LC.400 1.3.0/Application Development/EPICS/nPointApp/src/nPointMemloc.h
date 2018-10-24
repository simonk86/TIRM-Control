#ifndef nPointMemloc_H
#define nPointMemloc_H

/* 
   Define values used with the nPoint LC.400 controller
   Mainly related to memory locations and commands
*/

/* Memory Read/Write Commands */
#define messterm 0x55
#define ReadSingleLoc 0xA0
#define WriteSingleLoc 0xA2
#define WriteNext 0xA3
#define ReadArray 0xA4

/* Channel Base Memory Addresses */
#define Ch1Base 0x11831000
#define Ch2Base 0x11832000
#define Ch3Base 0x11833000

/* Static positioning addresses */
#define StageRange 0x078
#define RangeType  0x044
#define PosReq     0x218
#define PosRead    0x334

/* Control Loop Addresses */
#define ServoState 0x084
#define Pgain      0x720
#define Igain      0x728
#define Dgain      0x730

/* Wavetable Addresses */
#define WTenable   0x1F4
#define WTindex    0x1F8
#define WTDelay    0x200
#define WTendindex 0x204
#define WTactive   0x208

/* Wavetable Base Addresses */
#define Ch1WTBase 0xC0000000
#define Ch2WTBase 0xC0054000
#define Ch3WTBase 0xC00A8000
