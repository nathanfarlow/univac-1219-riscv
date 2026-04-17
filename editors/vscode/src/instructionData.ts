// Instruction help data extracted from the Emacs mode
// y = 12-bit memory location, k = 6-bit immediate, u = 12-bit immediate

export interface InstructionInfo {
    name: string;
    args: string;
    timing: string;
    description: string;
    fullHelp: string;
}

const rawHelp: Record<string, string> = {
    "CMAL":    "<y> - [4µs] Compare AL with *y, set flags",
    "CMALB":   "<y> - [4µs] Compare AL with *(y+B), set flags",
    "SLSU":    "<y> - [4µs] Selective substitute: AL = (~AU & AL) | (AU & *y)",
    "SLSUB":   "<y> - [4µs] Selective substitute with B addressing",
    "CMSK":    "<y> - [4µs] Compare (AL & AU) with (*y & AU), set flags",
    "CMSKB":   "<y> - [4µs] Compare (AL & AU) with (*(y+B) & AU), set flags",
    "ENTAU":   "<y> - [4µs] Load AU from memory: AU = *y",
    "ENTAUB":  "<y> - [4µs] Load AU from memory: AU = *(y+B)",
    "ENTAL":   "<y> - [4µs] Load AL from memory: AL = *y",
    "ENTALB":  "<y> - [4µs] Load AL from memory: AL = *(y+B)",
    "ADDAL":   "<y> - [4µs] Add to AL: AL += *y",
    "ADDALB":  "<y> - [4µs] Add to AL: AL += *(y+B)",
    "SUBAL":   "<y> - [4µs] Subtract from AL: AL -= *y",
    "SUBALB":  "<y> - [4µs] Subtract from AL: AL -= *(y+B)",
    "ADDA":    "<y> - [6µs] Add double word: A += (mem[y+1] << 18) | mem[y]",
    "ADDAB":   "<y> - [6µs] Add double word: A += (mem[y+1+B] << 18) | mem[y+B]",
    "SUBA":    "<y> - [6µs] Subtract double word: A -= (mem[y+1] << 18) | mem[y]",
    "SUBAB":   "<y> - [6µs] Subtract double word: A -= (mem[y+1+B] << 18) | mem[y+B]",
    "MULAL":   "<y> - [14µs] A = AL * *y",
    "MULALB":  "<y> - [14µs] A = AL * *(y+B)",
    "DIVA":    "<y> - [14µs] Divide: AL = A / *y; AU = A % *y",
    "DIVAB":   "<y> - [14µs] Divide: AL = A / *(y+B); AU = A % *(y+B)",
    "IRJP":    "<y> - [6µs] Indirect return jump: mem[*y] = PC+1; PC = *y+1",
    "IRJPB":   "<y> - [6µs] Indirect return jump: mem[*(y+B)] = PC+1; PC = *(y+B)+1",
    "ENTB":    "<y> - [4µs] Load B register: B = *y",
    "ENTBB":   "<y> - [4µs] Load B register: B = *(y+B)",
    "JP":      "<y> - [2µs] Unconditional jump: PC = y",
    "JPB":     "<y> - [2µs] Unconditional jump: PC = y+B",
    "ENTBK":   "<u> - [2µs] Load B with constant: B = u (sign extended)",
    "ENTBKB":  "<u> - [2µs] Add constant to B: B += u (sign extended)",
    "CL":      "<y> - [4µs] Clear memory: *y = 0",
    "CLB":     "<y> - [4µs] Clear memory: *(y+B) = 0",
    "STRB":    "<y> - [4µs] Store B: *y = B",
    "STRBB":   "<y> - [4µs] Store B: *(y+B) = B",
    "STRAL":   "<y> - [4µs] Store AL: *y = AL",
    "STRALB":  "<y> - [4µs] Store AL: *(y+B) = AL",
    "STRAU":   "<y> - [4µs] Store AU: *y = AU",
    "STRAUB":  "<y> - [4µs] Store AU: *(y+B) = AU",
    "SIN":     "[2µs] Set input active flag for channel k",
    "SOUT":    "[2µs] Set output active flag for channel k",
    "SEXF":    "[2µs] Set external function active flag for channel k",
    "IN":      "<k> - [6µs] Input transfer: setup DMA channel k",
    "OUT":     "<k> - [6µs] Output transfer: setup DMA channel k",
    "EXF":     "<k> - [6µs] External function: setup channel k",
    "RTC":     "[2µs] Enable real time clock monitor",
    "INSTP":   "<k> - [2µs] Stop input: clear input flag for channel k",
    "OUTSTP":  "<k> - [2µs] Stop output: clear output flag for channel k",
    "EXFSTP":  "<k> - [2µs] Stop external function: clear flag for channel k",
    "SRSM":    "<k> - [2µs] Set resume flag for channel group k",
    "SKPIIN":  "<k> - [2µs] Skip if input inactive on channel k",
    "SKPOIN":  "<k> - [2µs] Skip if output inactive on channel k",
    "SKPEIN":  "<k> - [2µs] Skip if external function inactive on channel k",
    "WTFI":    "[2µs] Wait for interrupt: halt until interrupt",
    "OUTOV":   "<k> - [2µs] Force output word on channel k",
    "EXFOV":   "<k> - [2µs] Force function word on channel k",
    "RIL":     "[2µs] Remove interrupt lockout (enable all interrupts)",
    "RXL":     "[2µs] Remove external interrupt lockout (enable external interrupts)",
    "SIL":     "[2µs] Disable all interrupts",
    "SXL":     "[2µs] Disable external interrupts",
    "RSHAU":   "<k> - [4+2*(k/4)µs] Right arithmetic shift AU by k bits",
    "RSHAL":   "<k> - [4+2*(k/4)µs] Right arithmetic shift AL by k bits",
    "RSHA":    "<k> - [4+2*(k/4)µs] Right arithmetic shift A by k bits",
    "SF":      "<k> - [4+2*(k/4)µs] Scale factor: normalize A register",
    "LSHAU":   "<k> - [4+2*(k/4)µs] Left circular shift AU by k bits",
    "LSHAL":   "<k> - [4+2*(k/4)µs] Left circular shift AL by k bits",
    "LSHA":    "<k> - [4+2*(k/4)µs] Left circular shift A by k bits",
    "SKP":     "<k> - [2µs] Skip if console key k is set",
    "SKPNBO":  "[2µs] Skip if no borrow flag",
    "SKPOV":   "[2µs] Skip if overflow flag set",
    "SKPNOV":  "[2µs] Skip if no overflow flag",
    "SKPODD":  "[2µs] Skip if (AL & AU) has odd bit count",
    "SKPEVN":  "[2µs] Skip if (AL & AU) has even bit count",
    "STOP":    "<k> - [2µs] Halt if console stop key k is set (k=40 unconditional)",
    "SKPNR":   "<k> - [2µs] Skip if no resume flag for channel group k",
    "RND":     "[2µs] Round: AL = AU + AL[17] if AU>=0, else AU - ~AL[17]",
    "CPAL":    "[2µs] Complement AL: AL = ~AL",
    "CPAU":    "[2µs] Complement AU: AU = ~AU",
    "CPA":     "[2µs] Complement A: A = ~A",
    "ENTICR":  "<k> - [2µs] Set index control register: ICR = k",
    "ENTSR":   "<k> - [2µs] Set special register: SR = k",
    "SLSET":   "<y> - [4µs] Bitwise OR: AL |= *y",
    "SLCL":    "<y> - [4µs] Bitwise AND: AL &= *y",
    "SLCP":    "<y> - [4µs] Bitwise XOR: AL ^= *y",
    "IJPEI":   "<y> - [4µs] Indirect jump & enable interrupts: PC = *y",
    "IJP":     "<y> - [4µs] Indirect jump: PC = *y",
    "BSK":     "<y> - [4µs] B loop: if (B == *y) skip; else B++",
    "ISK":     "<y> - [6µs] Index loop: if (*y == 0) skip; else (*y)--",
    "JPAUZ":   "<y> - [2µs] Jump if AU == 0: if (!AU) PC = y",
    "JPALZ":   "<y> - [2µs] Jump if AL == 0: if (!AL) PC = y",
    "JPAUNZ":  "<y> - [2µs] Jump if AU != 0: if (AU) PC = y",
    "JPALNZ":  "<y> - [2µs] Jump if AL != 0: if (AL) PC = y",
    "JPAUP":   "<y> - [2µs] Jump if AU >= 0: if (AU >= 0) PC = y",
    "JPALP":   "<y> - [2µs] Jump if AL >= 0: if (AL >= 0) PC = y",
    "JPAUNG":  "<y> - [2µs] Jump if AU < 0: if (AU < 0) PC = y",
    "JPALNG":  "<y> - [2µs] Jump if AL < 0: if (AL < 0) PC = y",
    "ENTALK":  "<u> - [2µs] Load AL with constant: AL = u (sign extended)",
    "ADDALK":  "<u> - [2µs] Add constant to AL: AL += u (sign extended)",
    "STRICR":  "<y> - [4µs] Store ICR bits: *y = (*y & 0xFFF0) | ICR",
    "BJP":     "<y> - [2µs] B loop jump: if (B--) PC = y",
    "STRADR":  "<y> - [4µs] Store address bits: *y = (*y & 0xF000) | (AL & 0xFFF)",
    "STRSR":   "<y> - [4µs] Store SR bits: *y = (*y & 0xFFC0) | SR; SR = 0",
    "RJP":     "<y> - [4µs] Call subroutine: mem[y] = PC+1; PC = y+1",
};

function parseHelp(name: string, raw: string): InstructionInfo {
    let args = "";
    let timing = "";
    let description = raw;

    // Extract args like <y>, <k>, <u>
    const argsMatch = raw.match(/^(<[^>]+>)\s*-\s*/);
    if (argsMatch) {
        args = argsMatch[1];
        description = raw.substring(argsMatch[0].length);
    }

    // Extract timing like [4µs]
    const timingMatch = description.match(/^\[([^\]]+)\]\s*/);
    if (timingMatch) {
        timing = timingMatch[1];
        description = description.substring(timingMatch[0].length);
    } else {
        // Some entries start with timing without args
        const timingMatch2 = raw.match(/^\[([^\]]+)\]\s*/);
        if (timingMatch2) {
            timing = timingMatch2[1];
            description = raw.substring(timingMatch2[0].length);
        }
    }

    return {
        name,
        args,
        timing,
        description,
        fullHelp: raw,
    };
}

export const instructions: Map<string, InstructionInfo> = new Map();
for (const [name, help] of Object.entries(rawHelp)) {
    instructions.set(name, parseHelp(name, help));
}

export const instructionNames: string[] = Object.keys(rawHelp);

export const directives: string[] = [
    "ORG", "SADD", "DATA", "AS", "AW", "EVEN", "END"
];

export const specialOperands: string[] = [
    "LOK"
];
