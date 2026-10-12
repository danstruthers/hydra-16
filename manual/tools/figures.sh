#!/bin/sh
# figures.sh PCB - the book's board drawings (manual/figures), from the board's layout file
PCB=${1:-../board/hydra-16.kicad_pcb}
D=$(dirname "$0")/..
B="node $D/tools/board.js"
$B "$PCB" "$D/figures/board-tour.svg" --key --names all \
 --color 'Processor:#e23d28:U1' --color 'Memory (RAM):#2a62b8:U7,U25,U27-U29,J1,J2' --color 'ROM:#3fa046:U6,U30-U37' \
 --color 'Registers and mirrors:#f2c12e:U48-U55,IC1-IC8' --color 'Input/output:#f08a24:U2,U3,U5,J3,J18-J25,J27,J28' \
 --color 'Sound:#9b59b6:U38,U41,U42,IC9,J26,J29' --color 'Slots:#999999:J12-J17' \
 --color 'Power and clock:#6b4f2a:J11,J10,U43,Y1,U39,U40,J5-J8'
# The building steps: the step's parts coloured, the rest faded
step() { n=$1; shift; $B "$PCB" "$D/figures/build-$n.svg" --fade --names colored "$@"; }
step 1 --color 'Shared RAM:#2a62b8:U25,U27-U29'
step 2 --color 'Resistors:#e23d28:R1-R7,R13,R19-R33' --color 'Resistor networks:#f08a24:RN1-RN4'
step 3 --color 'Ceramic capacitors:#f2c12e:C1-C50,C52,C57-C85'
step 4 --color 'Chip sockets:#3fa046:U1-U24,U26,U30-U42,U44-U61,IC1-IC9,J28'
step 5 --color 'Crystal and reset chip:#9b59b6:Y1,U43' --color 'Pin headers:#2a62b8:J5-J10,J27,J29,J30' --color 'Pin sockets:#f08a24:J1,J2,J18-J25'
step 6 --color 'Electrolytic capacitors:#e23d28:C51,C53-C56,C86-C93'
step 7 --color 'Connectors:#6b4f2a:J3,J11,J12-J17,J26'
# The memory cards (their layout files beside the main board's)
M=$(dirname "$PCB")/MemoryDaughterCard
$B "$M/MemDaughterCard.kicad_pcb" "$D/figures/memcard.svg" --key --names all \
 --color 'Connectors to the main board:#2a62b8:J1,J2' --color 'Module jumper:#e23d28:J3' --color 'RAM sockets:#3fa046:IC1-IC4'
$B "$M/MemDaughterCard_SurfaceMount.kicad_pcb" "$D/figures/memsmd.svg" --key --names all \
 --color 'Connectors to the main board:#2a62b8:J1,J2' --color 'Module jumpers:#e23d28:J3,J4' --color 'RAM chips:#3fa046:IC1-IC8'
